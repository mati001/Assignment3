/*
 *  kmeans.c
 *  Serial reference implementation of K-means on 2D points. Your job
 *  is to parallelize this file with OpenMP (see Assignment3.pdf).
 *
 *  Think about:
 *    - which loops are embarrassingly parallel
 *    - how to reduce per-cluster sums without serializing the workers
 *    - false sharing on the per-cluster accumulators
 *    - which scheduling clause fits each loop
 */

#include "kmeans.h"
#include <omp.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
int num_t = 1; //

PointSet *createPointSet(int numPoints)
// TODO : parrllel this?
{
    PointSet *p = (PointSet *)malloc(sizeof(PointSet));
    p->numPoints = numPoints;
    p->points = (Point *)malloc((size_t)numPoints * sizeof(Point));
    p->assignments = (int *)calloc((size_t)numPoints, sizeof(int));
    num_t = numPoints / 5000;
    return p;
}

void freePointSet(PointSet *p)
{
    if (p == NULL)
        return;
    free(p->points);
    free(p->assignments);
    free(p);
}

Centroids *createCentroids(int k)
{
    Centroids *c = (Centroids *)malloc(sizeof(Centroids));
    c->k = k;
    c->centroids = (Point *)malloc((size_t)k * sizeof(Point));
    return c;
}

void freeCentroids(Centroids *c)
{
    if (c == NULL)
        return;
    free(c->centroids);
    free(c);
}

static inline double squaredDistance(Point a, Point b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Assigns each point to the nearest centroid
// TODO : parrllel this
void assignPointsToClusters(PointSet *data, Centroids *centroids)
{
    int n = data->numPoints;
    int k = centroids->k;

#pragma omp for schedule(static)
    for (int i = 0; i < n; i++)
    {
        double bestDist = squaredDistance(data->points[i], centroids->centroids[0]);
        int bestCluster = 0;

        for (int c = 1; c < k; c++)
        {
            double d = squaredDistance(data->points[i], centroids->centroids[c]);
            if (d < bestDist)
            {
                bestDist = d;
                bestCluster = c;
            }
        }
        data->assignments[i] = bestCluster;
    }
}
// Resets the accumulation arrays

void resetAccumulators(int k, double *sumX, double *sumY, int *counts)
{
#pragma omp simd
    for (int c = 0; c < k; c++)
    {
        sumX[c] = 0.0;
        sumY[c] = 0.0;
        counts[c] = 0;
    }
}

// Accumulates point coordinates into their assigned clusters
void accumulateClusters(PointSet *data, double *sumX, double *sumY, int *counts, int k)
{
    int n = data->numPoints;
#pragma omp for reduction(+ : sumX[ : k], sumY[ : k], counts[ : k])
    for (int i = 0; i < n; i++)
    {
        int c = data->assignments[i];
        sumX[c] += data->points[i].x;
        sumY[c] += data->points[i].y;
        counts[c]++;
    }
}

// Updates centroids and returns the maximum movement distance
double updateCentroids(Centroids *centroids, double *sumX, double *sumY, int *counts, double *sharedMaxMovement)
{
    int k = centroids->k;
    double localMaxMovement = 0.0;
#pragma omp for schedule(dynamic) nowait // run througth all the centroid and update them and the max movement
    for (int c = 0; c < k; c++)
    {
        if (counts[c] == 0)
            continue;

        Point updated;
        updated.x = sumX[c] / counts[c];
        updated.y = sumY[c] / counts[c];

        double mv = squaredDistance(centroids->centroids[c], updated);
        if (mv > localMaxMovement)
        {
            localMaxMovement = mv;
        }
        centroids->centroids[c] = updated;
    }
    if (localMaxMovement > *sharedMaxMovement)
    {
#pragma omp critical
        {
            if (localMaxMovement > *sharedMaxMovement)
            {
                *sharedMaxMovement = localMaxMovement;
            }
        }
    }
#pragma omp barrier
    return *sharedMaxMovement;
}
void assignAndAccumulate(PointSet *data, Centroids *centroids,
                         double *sumX, double *sumY, int *counts, int k)
{
    int n = data->numPoints;

// Pass 1: assign (no race — each thread writes its own i)
#pragma omp for schedule(static)
    for (int i = 0; i < n; i++)
    {
        double bestDist = squaredDistance(data->points[i], centroids->centroids[0]);
        int bestCluster = 0;
        for (int c = 1; c < k; c++)
        {
            double d = squaredDistance(data->points[i], centroids->centroids[c]);
            if (d < bestDist)
            {
                bestDist = d;
                bestCluster = c;
            }
        }
        data->assignments[i] = bestCluster;
    }
// implicit barrier — assignments[] fully written before accumulation

// Pass 2: accumulate with reduction (private copies, no contention)
#pragma omp for schedule(static) reduction(+ : sumX[ : k], sumY[ : k], counts[ : k])
    for (int i = 0; i < n; i++)
    {
        int c = data->assignments[i];
        sumX[c] += data->points[i].x;
        sumY[c] += data->points[i].y;
        counts[c]++;
    }
}

int runKMeans(PointSet *data, Centroids *centroids, int maxIters, double tolerance)
{
    int k = centroids->k;

    double *sumX = (double *)malloc((size_t)k * sizeof(double));
    double *sumY = (double *)malloc((size_t)k * sizeof(double));
    int *counts = (int *)malloc((size_t)k * sizeof(int));

    const double tolSquared = tolerance * tolerance;
    int final_iter = 0;
    double sharedMaxMovement = 0.0;
    int iter = 0;
    int converged = 0;

#pragma omp parallel shared(iter, converged, sharedMaxMovement)
    {
        while (1)
        {
// One thread drives the loop counter and convergence flag
#pragma omp single
            {
                resetAccumulators(k, sumX, sumY, counts);
                sharedMaxMovement = 0.0;
            }
            if (iter >= maxIters)
                break;
            assignAndAccumulate(data, centroids, sumX, sumY, counts, k);
            updateCentroids(centroids, sumX, sumY, counts, &sharedMaxMovement);
#pragma omp single // One thread updates iter and checks convergence
            {
                iter++;
                if (sharedMaxMovement < tolSquared)
                    converged = 1;
            }
            if (converged)
                break;
        }
    }
    final_iter = iter;
    free(sumX);
    free(sumY);
    free(counts);

    return final_iter;
}
