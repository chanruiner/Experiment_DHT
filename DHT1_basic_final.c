#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ID_LENGTH 20
#define BUCKET_SIZE 3
#define BUCKET_COUNT (8 * ID_LENGTH)

// Node结构
typedef struct Node {
    uint8_t id[ID_LENGTH];
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

// 初始化K_Bucket
void init_k_bucket(K_Bucket* kb) {
    for (int i = 0; i < BUCKET_COUNT; ++i) {
        kb->buckets[i].head = NULL;
        kb->buckets[i].count = 0;
    }
}

// 计算两个ID之间的异或距离
void xor_distance(const uint8_t* id1, const uint8_t* id2, uint8_t* out) {
    for (int i = 0; i < ID_LENGTH; ++i) {
        out[i] = id1[i] ^ id2[i];
    }
}

// 计算前导零位数
int leading_zeros(const uint8_t* id) {
    int count = 0;
    for (int i = 0; i < ID_LENGTH; ++i) {
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
int get_bucket_index(const uint8_t* local_id, const uint8_t* remote_id) {
    uint8_t xor_result[ID_LENGTH];
    xor_distance(local_id, remote_id, xor_result);
    return leading_zeros(xor_result);
}

// 插入节点
void insert_node(K_Bucket* kb, const uint8_t* local_id, const uint8_t* node_id) {
    int index = get_bucket_index(local_id, node_id);
    Bucket* bucket = &kb->buckets[index];

    if (bucket->count < BUCKET_SIZE) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        memcpy(new_node->id, node_id, ID_LENGTH);
        new_node->next = bucket->head;
        bucket->head = new_node;
        bucket->count++;
    } else {
        // 桶已满，根据Kademlia协议进行适当的操作，如替换最旧节点
        // Check if the node already exists in the bucket
Node* node = bucket->head;
if (memcmp(node->id, node_id, ID_LENGTH) == 0) {
// If the node already exists, move it to the front of the bucket
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
// If the node does not exist, ping the least recently seen node to see if it is still alive
// If the least recently seen node does not respond, replace it with the new node
// Otherwise, drop the new node
}

// 打印每个桶中存在的NodeID
void print_bucket_contents(K_Bucket* kb) {
    for (int i = 0; i < BUCKET_COUNT; ++i) {
        Bucket* bucket = &kb->buckets[i];
        printf("Bucket %d:\n", i);
        Node* node = bucket->head;
        while (node != NULL) {
            for (int j = 0; j < ID_LENGTH; ++j) {
                printf("%02x", node->id[j]);
            }
            printf("\n");
            node = node->next;
        }
    }
}

int main() {
    K_Bucket kb;
    init_k_bucket(&kb);

    // 假设本地节点ID
    uint8_t local_id[ID_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB};

    // 插入节点
    uint8_t node_id1[ID_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB};
    uint8_t node_id2[ID_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBC};
    uint8_t node_id3[ID_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBD};
    uint8_t node_id4[ID_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBE};

    insert_node(&kb, local_id, node_id1);
    insert_node(&kb, local_id, node_id2);
    insert_node(&kb, local_id, node_id3);
    insert_node(&kb, local_id, node_id4);

    // 打印每个桶中存在的NodeID
    print_bucket_contents(&kb);

    return 0;
}