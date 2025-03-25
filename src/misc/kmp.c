#include <stdio.h>
#include <string.h>

void compute_next(const char* P, int* next, int m) {
    next[0] = -1;
    int k = -1, j = 0;
    while (j < m-1) {
        if (k == -1 || P[j] == P[k]) {
            j++;
            k++;
            next[j] = k;
        } else {
            k = next[k];
        }
    }
}

int kmp_search(const char* S, const char* P, int* next) {
    int n = strlen(S), m = strlen(P);
    int i = 0, j = 0;
    while (i < n && j < m) {
        if (j == -1 || S[i] == P[j]) {
            i++;
            j++;
        } else {
            j = next[j];
        }
    }
    return (j == m) ? (i - m) : -1;
}

int kmpmain() {
    char S[] = "ABABDABABC";
    char P[] = "ABABC";
    int m = strlen(P);
    int next[m];
    compute_next(P, next, m);
    int pos = kmp_search(S, P, next);
    if (pos != -1) {
        printf("Pattern found at position %d\n", pos);
    } else {
        printf("Pattern not found\n");
    }
    return 0;
}