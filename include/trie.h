#ifndef _THOR_TRIE_H_
#define _THOR_TRIE_H_
#include "common.h"
#include "scanner.h"

#define ALPHABET_SIZE 26

typedef struct TrieNode
{
    bool is_end_of_word;
    TokenType type;
    struct TrieNode *children[ALPHABET_SIZE];
} TrieNode;

typedef struct Trie
{
    TrieNode *root;
} Trie;

typedef struct SearchResult
{
    TokenType type;
    bool found;
} SearchResult;

TrieNode *create_node();

void init_trie(Trie *trie);
void insert_trie(Trie *trie, const char *word, TokenType type);
SearchResult search_trie(Trie *trie, const char *word);
void free_trie_node(TrieNode *node);
void free_trie(Trie *trie);

#endif // _THOR_TRIE_H_