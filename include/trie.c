#include "trie.h"

TrieNode *create_node()
{
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    node->is_end_of_word = false;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        node->children[i] = NULL;
    return node;
}

void init_trie(Trie *trie)
{
    trie->root = create_node();
}

void insert_trie(Trie *trie, const char *word, TokenType type)
{
    TrieNode *current = trie->root;
    for (int i = 0; word[i] != '\0'; i++)
    { // Assuming the input is lowercase
        int index = word[i] - 'a';
        if (current->children[index] == NULL)
            current->children[index] = create_node();
        current = current->children[index];
    }
    current->is_end_of_word = true;
    current->type = type;
}

SearchResult search_trie(Trie *trie, const char *word)
{
    SearchResult result;
    TrieNode *current = trie->root;
    for (int i = 0; word[i] != '\0'; i++)
    {
        // Assuming the input is lowercase
        int index = word[i] - 'a';
        if (current->children[index] == NULL)
        {
            result.found = false;
            result.type = TOKEN_IDENTIFIER;
            return result;
        }
        current = current->children[index];
    }
    result.type = current->type;
    result.found = current != NULL && current->is_end_of_word;
    return result;
}

void free_trie(Trie *trie)
{
    free_trie_node(trie->root);
}

void free_trie_node(TrieNode *node)
{
    if (node == NULL)
        return;
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        free_trie_node(node->children[i]);
    }
    free(node);
}