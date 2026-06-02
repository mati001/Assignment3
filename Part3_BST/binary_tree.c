#include "binary_tree.h"
#include <stdio.h>
#include <stdlib.h>

TreeNode *createNode(int data)
{
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node != NULL)
    {
        node->data = data;
        node->left = NULL;
        node->right = NULL;
        omp_init_lock(&node->lock);
    }
    return node;
}

TreeNode *findMin(TreeNode *root)
{
    if (root == NULL)
    {
        return NULL;
    }
    omp_set_lock(&root->lock);
    while (root->left != NULL)
    {
        omp_set_lock(&root->left->lock);
        omp_unset_lock(&root->lock);
        root = root->left;
    }
    omp_unset_lock(&root->lock);
    return root;
}

bool searchNodeHelper(TreeNode *root, int data)
{
    if (root == NULL)
    {
        return false;
    }
    if (root->data == data)
    {
        omp_unset_lock(&root->lock);
        return true;
    }
    if (data < root->data)
    {
        if (root->left == NULL)
        {
            omp_unset_lock(&root->lock);
            return false;
        }
        omp_set_lock(&root->left->lock);
        omp_unset_lock(&root->lock);
        return searchNodeHelper(root->left, data);
    }
    else
    {
        if (root->right == NULL)
        {
            omp_unset_lock(&root->lock);
            return false;
        }
        omp_set_lock(&root->right->lock);
        omp_unset_lock(&root->lock);
        return searchNodeHelper(root->right, data);
    }
}
/* Public API function */
bool searchNode(TreeNode *root, int data)
{
    if (root == NULL)
    {
        return false;
    }
    // Lock the root node before passing it to the real function
    omp_set_lock(&root->lock);
    return searchNodeHelper(root, data);
}

void insertNodeHelper(TreeNode *root, int data)
{
    if (data < root->data)
    {
        if (root->left == NULL)
        {
            root->left = createNode(data);
            omp_unset_lock(&root->lock);
            return;
        }
        omp_set_lock(&root->left->lock);
        omp_unset_lock(&root->lock);
        insertNodeHelper(root->left, data);
    }
    else if (data > root->data)
    {
        if (root->right == NULL)
        {
            root->right = createNode(data);
            omp_unset_lock(&root->lock);
            return;
        }
        omp_set_lock(&root->right->lock);
        omp_unset_lock(&root->lock);
        insertNodeHelper(root->right, data);
    }
    else
    {
        omp_unset_lock(&root->lock); // this is in case that the data is duplicate so we do nothing
    }
}
// TODO add a lock
TreeNode *insertNode(TreeNode *root, int data)
{
    if (root == NULL)
    {
        return createNode(data);
    }
    omp_set_lock(&root->lock);
    insertNodeHelper(root, data);
    return root;
}

TreeNode *deleteNodeHelper(TreeNode *root, int data)
{

    if (data < root->data)
    {
        root->left = deleteNode(root->left, data);
    }
    else if (data > root->data)
    {
        root->right = deleteNode(root->right, data);
    }
    else
    {
        // Node with only one child or no child
        if (root->left == NULL)
        {
            TreeNode *temp = root->right;
            free(root);
            return temp;
        }
        else if (root->right == NULL)
        {
            TreeNode *temp = root->left;
            free(root);
            return temp;
        }

        // Node with two children: Get the inorder successor (smallest in the right subtree)
        TreeNode *temp = findMin(root->right);

        // Copy the inorder successor's content to this node
        root->data = temp->data;

        // Delete the inorder successor
        root->right = deleteNode(root->right, temp->data);
    }
    return root;
}

// TODO add a lock
TreeNode *deleteNode(TreeNode *root, int data)
{
    if (root == NULL)
    {
        return NULL;
    }
    omp_set_lock(&root->lock);

    root = deleteNodeHelper(root, data);

    return root;
}

// TODO: add a lock
void inorderTraversal(TreeNode *root)
{
    if (root != NULL)
    {
        inorderTraversal(root->left);
        printf("%d ", root->data);
        inorderTraversal(root->right);
    }
}

void freeTree(TreeNode *root)
{
    if (root != NULL)
    {
        freeTree(root->left);
        freeTree(root->right);
        omp_destroy_lock(&root->lock);
        free(root);
    }
}
