#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define ID_LENG 20
#define BUCKET_SIZE 3
#define BUCKET_COUNT (8 * ID_LENG)

// Node结构
typedef struct Node {
    uint8_t id[ID_LENG];
    struct Node* next;
} Node;

// Bucket结构
typedef struct Bucket {
    Node* head;
    int count;
} Bucket;

// K_Bucket结构
typedef struct K_Bucket {
    Bucket buckets[BUCKET_COUNT];
} K_Bucket;

// Peer结构
typedef struct Peer {
    uint8_t id[ID_LENG];
    K_Bucket k_bucket;
} Peer;

// 初始化K_Bucket
void init_k_bucket(K_Bucket* kb) {
    for (int i = 0; i < BUCKET_COUNT; ++i) {
        kb->buckets[i].head = NULL;
        kb->buckets[i].count = 0;
    }
}

// 计算两个ID之间的异或距离
void xor_distance(const uint8_t* id1, const uint8_t* id2, uint8_t* out) {
    for (int i = 0; i < ID_LENG; ++i) {
        out[i] = id1[i] ^ id2[i];
    }
}

// 计算前导零位数
int leading_zeros(const uint8_t* id) {
    int count = 0;
    for (int i = 0; i < ID_LENG; ++i) {
        if (id[i] == 0) {
            count += 8;
        } else {
            for (int j = 7; j >= 0; --j) {
                if ((id[i] & (1 << j)) == 0) {
                    count++;
                } else {
                    break;
                }
            }
            break;
        }
    }
    return count;
}

// 根据节点ID将其分配到正确的桶中
int get_index(const uint8_t* local_id, const uint8_t* remote_id) {
    uint8_t xor_result[ID_LENG];
    xor_distance(local_id, remote_id, xor_result);
    return leading_zeros(xor_result);
}

// 插入节点
void InsertNode(K_Bucket* kb, const uint8_t* local_id, const uint8_t* node_id) {
    int index = get_index(local_id, node_id);
    Bucket* bucket = &kb->buckets[index];

    if (bucket->count < BUCKET_SIZE) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        memcpy(new_node->id, node_id, ID_LENG);
        new_node->next = bucket->head;
        bucket->head = new_node;
        bucket->count++;
    } else {
        // 桶已满，根据Kademlia协议进行适当的操作，如替换最旧节点
        Node* node = bucket->head;
        if (memcmp(node->id, node_id, ID_LENG) == 0) {
            if (node != bucket->head) {
                Node* prev = bucket->head;
                while (prev->next != node) {
                    prev = prev->next;
                }
                prev->next = node->next;
                node->next = bucket->head;
                bucket->head = node;
            }
            return;
        }
        node = node->next;
    }
}

// 查找节点
Node** FindNode(K_Bucket* kb, const uint8_t* local_id, const uint8_t* node_id) {
    int index = get_index(local_id, node_id);
    Bucket* bucket = &kb->buckets[index];
    Node* node = bucket->head;
    Node** result = (Node**)malloc(2 * sizeof(Node*));

    int count = 0;
    while (node != NULL) {
        if (memcmp(node->id, node_id, ID_LENG) == 0) {
            result[0] = node;
            result[1] = NULL;
            return result;
        }
        node = node->next;
        count++;
    }
    node = bucket->head;
    for (int i = 0; i < count; ++i) {
        result[i] = node;
        node = node->next;
    }
    return result;
}

// 初始化节点ID
void init_id(uint8_t* id) {
    for (int i = 0; i < ID_LENG; ++i) {
        id[i] = rand() % 256;
    }
}

// 打印节点ID
void print_id(const uint8_t* id) {
    for (int i = 0; i < ID_LENG; ++i) {
        printf("%02x", id[i]);
    }
    printf("\n");
}

// 打印桶的节点信息
void print_bucket(const Bucket* bucket) {
    Node* node = bucket->head;
    while (node != NULL) {
        print_id(node->id);
        node = node->next;
    }
}

int main() {
    srand(time(NULL));

    Peer peers[5];
    for (int i = 0; i < 5; ++i) {
        init_id(peers[i].id);
        init_k_bucket(&peers[i].k_bucket);
    }

    // 生成200个新的Peer
    for (int i = 0; i < 200; ++i) {
        uint8_t new_peer_id[ID_LENG];
        init_id(new_peer_id);

        printf("New Peer ID: ");
        print_id(new_peer_id);
        printf("\n");

        // 将新节点广播到所有已知节点
        for (int j = 0; j < 5; ++j) {
            InsertNode(&peers[j].k_bucket, peers[j].id, new_peer_id);
        }
    }

    // 打印桶的信息
    for (int i = 0; i < 5; ++i) {
        printf("Peer %d K-Buckets:\n", i + 1);
        for (int j = 0; j < BUCKET_COUNT; ++j) {
            printf("Bucket %d:\n", j);
            print_bucket(&peers[i].k_bucket.buckets[j]);
        }
        printf("\n");
    }

    return 0;
}