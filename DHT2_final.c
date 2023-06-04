#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "sha1.h" 

#define PEERS 100 //总100个peer
#define BUCKETS 160 //总160个桶
#define BUCKET_SIZE 3

typedef struct Node {
    uint8_t id[BUCKET_SIZE];
    struct Node* next;
} Node;

typedef struct Bucket { 
    Node* bucket[BUCKET_SIZE];
    int size;
} Bucket;

typedef struct PeerID {
    uint8_t id[20]; 
} PeerID;

typedef struct KeyValuePair {
    PeerID key;
    uint8_t value[32]; 
} KeyValuePair;

typedef struct K_BUCKET {
    PeerID peer_id;
    KeyValuePair stored_values[BUCKETS];
    Bucket buckets[BUCKETS];
    int num_values;
} K_BUCKET;

typedef struct Peer {
    PeerID peer_id;
    K_BUCKET *k_bucket;
} Peer;

uint32_t XORdistance(PeerID *a, PeerID *b) {
    uint32_t distance = 0;
    for (int i = 0; i < 20; i++) {
        distance += (a->id[i] ^ b->id[i]) << (8 * (19 - i));
    }
    return distance;
}

int BucketIndex(PeerID *peer_id, PeerID *key) {
    uint32_t dist = XORdistance(peer_id, key);
    int i;
    for (i = 0; i < BUCKETS - 1; i++) {
        if (dist < (1 << (BUCKETS - i - 1))) {
            break;
        }
    }
    return i;
}

//随机生成String
void RandomString(uint8_t *str, size_t length) {
    static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < length; i++) {
        str[i] = characters[rand() % (sizeof(characters) - 1)];
    }
    str[length] = '\0';
}

//根据节点到目标节点的距离对节点进行排序
void Sort(Peer **peers, int size, PeerID *key) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            uint32_t dist_i = XORdistance(&peers[i]->peer_id, key);
            uint32_t dist_j = XORdistance(&peers[j]->peer_id, key);
            if (dist_i > dist_j) {
                Peer *tmp = peers[i];
                peers[i] = peers[j];
                peers[j] = tmp;
            }
        }
    }
}

void FindNode(K_BUCKET *k_bucket, uint8_t key[], Peer *closest_peers[]) {
    // 初始化结果数组
    for (int i = 0; i < 2; i++) {
        closest_peers[i] = NULL;
    }

    // 查找包含最近节点的桶
    int bucket_index = BucketIndex(&k_bucket->peer_id, (PeerID *)key);
    Bucket *bucket = &k_bucket->buckets[bucket_index];

    // 如果桶中节点数量小于k，将所有节点添加到结果数组
    if (bucket->size <= 2) {
        for (int i = 0; i < bucket->size; i++) {
            closest_peers[i] = (Peer *)&bucket->bucket[i]->id;
        }
    } else {
        // 否则，对桶中的节点进行排序，选择距离最近的k个节点
        Peer *sorted_peers[BUCKET_SIZE];
        for (int i = 0; i < bucket->size; i++) {
            sorted_peers[i] = (Peer *)&bucket->bucket[i];
        }
        Sort(sorted_peers, bucket->size, (PeerID *)key);
        for (int i = 0; i < 2; i++) {
            closest_peers[i] = sorted_peers[i];
        }
    }
}

_Bool SetValue(K_BUCKET *k_bucket, uint8_t key[], uint8_t value[]) {
   uint8_t hash[SHA1_DIGEST_SIZE];
   SHA1_CTX ctx;
   sha1_init(&ctx);
   sha1_update(&ctx, value, 32);
   sha1_final(&ctx, hash);
   if (memcmp(key, hash, 20) != 0) {
       return false;
   }
    for (int i = 0; i < k_bucket->num_values; i++) {
        if (memcmp(k_bucket->stored_values[i].key.id, key, 20) == 0) {
            return true;
        }
    }

    memcpy(k_bucket->stored_values[k_bucket->num_values].key.id, key, 20);
    memcpy(k_bucket->stored_values[k_bucket->num_values].value, value, 32);
    k_bucket->num_values++;

    int bucket_index = BucketIndex(&k_bucket->peer_id, (PeerID *)key);
    Bucket *bucket = &k_bucket->buckets[bucket_index];

    Peer *closest_peers[2];
    FindNode(k_bucket, key, closest_peers);

    for (int i = 0; i < 2; i++) {
        if (closest_peers[i] != NULL) {
            SetValue(closest_peers[i]->k_bucket, key, value);
        }
    }
    return true;
}

uint8_t *GetValue(K_BUCKET *k_bucket, uint8_t key[]) {
       for (int i = 0; i < k_bucket->num_values; i++) {
        if (memcmp(k_bucket->stored_values[i].key.id, key, 20) == 0) {
            return k_bucket->stored_values[i].value;
        }
    }

    Peer *closest_peers[2];
    FindNode(k_bucket, key, closest_peers);

    for (int i = 0; i < 2; i++) {
        if (closest_peers[i] != NULL) {
            uint8_t *value = GetValue(closest_peers[i]->k_bucket, key);
            if (value != NULL) {
                
                uint8_t hash[SHA1_DIGEST_SIZE];
                SHA1_CTX ctx;
                sha1_init(&ctx);
                sha1_update(&ctx, value, 32);
                sha1_final(&ctx, hash);
                if (memcmp(key, hash, 20) == 0) {
                    return value;
                } else {
                    return NULL;
                }
            }
        }
    }
}



// uint8_t *GetValue(DHT *dht, uint8_t key[]) {
//     // 1. 判断当前的Key自己这个Peer是否已经存储对应的value
//     for (int i = 0; i < dht->num_values; i++) {
//         if (memcmp(dht->stored_values[i].key.id, key, 20) == 0) {
//             return dht->stored_values[i].value;
//         }
//     }

//     // 2. 对当前的Key执行⼀次FindNode函数
//     Peer *closest_peers[K_VALUE];
//     FindNode(dht, key, closest_peers, K_VALUE);

//     // 然后对这两个Peer执行GetValue操作
//     for (int i = 0; i < K_VALUE; i++) {
//         if (closest_peers[i] != NULL) {
//             uint8_t *value = GetValue(closest_peers[i]->dht, key);
//             if (value != NULL) {
//                 // 返回校验成功之后的value
//                 // 此处略过校验过程，因为涉及到具体的hash实现
//                 return value;
//             }
//         }
//     }
//     return NULL;
// }


// int main() {
//     // 初始化100个节点，每个节点的 PeerID 和 DHT 结构体都随机生成
//     Peer peers[NUM_PEERS];
//     DHT dhts[NUM_PEERS];
//     for (int i = 0; i < NUM_PEERS; i++) {
//         PeerID peer_id;
//         srand(time(NULL) + i); // 用时间和节点编号作为随机种子
//         for (int j = 0; j < 20; j++) {
//             peer_id.id[j] = rand() % 256;
//         }
//         peers[i].peer_id = peer_id;
//         peers[i].dht = &dhts[i];

//         // 将节点添加到对应的桶中
//         int bucket_index = find_bucket_index(&peer_id, &peer_id);
//         Bucket *bucket = &dhts[i].buckets[bucket_index];
//         if (bucket->size < MAX_BUCKET_SIZE) {
//             bucket->bucket[bucket->size] = peers[i];
//             bucket->size++;
//         }
//     }

//     // 随机生成200个字符串，计算它们的哈希值，随机选择节点进行 SetValue 操作，并记录 Key
//     uint8_t keys[200][20];
//     uint8_t values[200][32];
//     srand(time(NULL)); // 用当前时间作为随机种子
//     for (int i = 0; i < 200; i++) {
//         // 随机生成字符串并计算哈希值
//         uint8_t key[20];
//         uint8_t value[32];
//         for (int j = 0; j < 20; j++) {
//             key[j] = rand() % 256;
//         }
//         for (int j = 0; j < 32; j++) {
//             value[j] = rand() % 256;
//         }
//         // 将键值对保存到随机选择的节点上
//         int peer_index = rand() % NUM_PEERS;
//         SetValue(&dhts[peer_index], key, value);
//         memcpy(keys[i], key, 20);
//         memcpy(values[i], value, 32);
//     }

//     // 随机选择100个 Key，并从随机的节点上获取对应的值
//     srand(time(NULL) + 1); // 修改随机种子，确保每次运行结果不同
//     for (int i = 0; i < 100; i++) {
//         int key_index = rand() % 200;
//         uint8_t *value = GetValue(&dhts[rand() % NUM_PEERS], keys[key_index]);
//         if (value != NULL) {
//             printf("Key %d: ", key_index);
//             for (int j = 0; j < 20; j++) {
//                 printf("%02x", keys[key_index][j]);
//             }
//             printf(" => ");
//             for (int j = 0; j < 32; j++) {
//                 printf("%02x", value[j]);
//             }
//             printf("\n");
//         } else {
//             printf("Failed to get value for key %d\n", key_index);
//         }
//     }

//     return 0;
// }


// int main() {
//     srand(time(NULL));
//     
//     // 初始化100个节点
//     DHT dhts[NUM_PEERS];
//     for (int i = 0; i < NUM_PEERS; i++) {
//         PeerID peer_id;
//         for (int j = 0; j < 20; j++) {
//             peer_id.id[j] = rand() % 256;
//         }
//         dhts[i].peer_id = peer_id;
//     }

//     // 将每个节点的 PeerID 放入对应的桶中
//     for (int i = 0; i < NUM_PEERS; i++) {
//         for (int j = 0; j < NUM_PEERS; j++) {
//             if (i == j) {
//                 continue;
//             }
//             int bucket_index = find_bucket_index(&dhts[i].peer_id, &dhts[j].peer_id);
//             Bucket *bucket = &dhts[i].buckets[bucket_index];
//             if (bucket->size < MAX_BUCKET_SIZE) {
//                 Peer *peer = (Peer *)malloc(sizeof(Peer));
//                 peer->peer_id = dhts[j].peer_id;
//                 peer->dht = &dhts[j];
//                 bucket->bucket[bucket->size++] = (Node *)peer;
//             }
//         }
//     }

//     // 随机生成200个字符串，计算哈希值，随机找一个节点进行 SetValue 操作
//     uint8_t keys[200][20];
//     uint8_t values[200][32];
//     for (int i = 0; i < 200; i++) {
//         uint8_t key[20];
//         uint8_t value[32];
//         for (int j = 0; j < 20; j++) {
//             key[j] = rand() % 256;
//         }
//         for (int j = 0; j < 32; j++) {
//             value[j] = rand() % 256;
//         }
//         SHA1_CTX ctx;
//         sha1_init(&ctx);
//         sha1_update(&ctx, value, SHA1_DIGEST_SIZE);
//         sha1_final(&ctx, keys[i]);

//         SetValue(&dhts[rand() % NUM_PEERS], key, value);
//         memcpy(keys[i], key, 20);
//         memcpy(values[i], value, 32);
//     }

//     // 随机选择100个 key 进行 GetValue 操作
//     for (int i = 0; i < 100; i++) {
//         uint8_t *key = keys[rand() % 200];
//         uint8_t *value = GetValue(&dhts[rand() % NUM_PEERS], key);
//         if (value != NULL) {
//             printf("GetValue(%d): ", i);
//             for (int j = 0; j < 20; j++) {
//                 printf("%02x", key[j]);
//             }
//             printf(" -> ");
//             for (int j = 0; j < 32; j++) {
//                 printf("%02x", value[j]);
//             }
//             printf("\n");
//         }
//     }
//      return 0;
// }


// int main() {
    
//     // 初始化100个Peer节点
//     Peer peers[NUM_PEERS];
//     for (int i = 0; i < NUM_PEERS; i++) {
//         // 初始化PeerID
//         PeerID peer_id;
//         for (int j = 0; j < 20; j++) {
//             peer_id.id[j] = rand() % 256;
//         }

//         // 初始化DHT
//         DHT *dht = (DHT *)malloc(sizeof(DHT));
//         dht->peer_id = peer_id;
//         dht->num_values = 0;
//         for (int j = 0; j < NUM_BUCKETS; j++) {
//             dht->buckets[j].size = 0;
//         }

//         peers[i].peer_id = peer_id;
//         peers[i].dht = dht;
//     }

//     // 随机生成200个字符串，并计算出它们的Hash
//     char keys[200][256];
//     uint8_t values[200][32];
//     for (int i = 0; i < 200; i++) {
//         // 生成随机字符串
//         int len = rand() % 255 + 1;
//         for (int j = 0; j < len; j++) {
//             keys[i][j] = rand() % 26 + 'a';
//         }
//         keys[i][len] = '\0';

//         // 计算Hash
//         uint8_t hash1[SHA1_DIGEST_SIZE];
//         SHA1_CTX ctx;
//         sha1_init(&ctx);
//         sha1_update(&ctx, keys[i], strlen(keys[i]));
//         sha1_final(&ctx, hash1);

//         memcpy(values[i], hash1, 20);
//     }
   
//     // 随机选择100个Key并执行GetValue操作
//     int indices[100];
//     for (int i = 0; i < 100; i++) {
//         indices[i] = rand() % 200;
//     }

//     for (int i = 0; i < 100; i++) {
//         uint8_t *value = GetValue(peers[rand() % NUM_PEERS].dht, values[indices[i]]);
//         if (value != NULL) {
//             printf("%s: ", keys[indices[i]]);
//             for (int j = 0; j < 32; j++) {
               
//                 printf("%02x", value[j]);
//             }
//             printf("\n");
//         } else {
//             printf("not found\n");
//         }
//     }

//     // // 随机选择一个节点并执行SetValue操作
//     // int index = rand() % NUM_PEERS;
//     // SetValue(peers[index].dht, values[rand() % 200], keys[rand() % 200]);

//     return 0;
// }

int main() {
    srand(time(NULL));
    // 初始化100个Peer节点
   Peer peers[PEERS];
     for (int i = 0; i < PEERS; i++) {
        // 初始化PeerID
        PeerID peer_id;
        for (int j = 0; j < 20; j++) {
            peer_id.id[j] = rand() % 256;
        }

        // 初始化K_BUCKET
        K_BUCKET *k_bucket = (K_BUCKET *)malloc(sizeof(K_BUCKET));
        k_bucket->peer_id = peer_id;
        k_bucket->num_values = 0;
        for (int j = 0; j < BUCKETS; j++) {
            k_bucket->buckets[j].size = 0;
        }

        peers[i].peer_id = peer_id;
        peers[i].k_bucket = k_bucket;
    }

    uint8_t keys[200][20];
    for (int i = 0; i < 200; i++) {
        uint8_t random_string[33];
        RandomString(random_string, 32);

        uint8_t key[20];
        SHA1_CTX ctx;
        sha1_init(&ctx);
        sha1_update(&ctx, random_string, 32);
        sha1_final(&ctx, key);

        memcpy(keys[i], key, 20);

        int random_peer_index = rand() % PEERS;
        SetValue(peers[random_peer_index].k_bucket, key, random_string);
    }

   
    uint8_t selected_keys[100][20];
    for (int i = 0; i < 100; i++) {
        int random_key_index = rand() % 200;
        memcpy(selected_keys[i], keys[random_key_index], 20);

        int random_peer_index = rand() % PEERS;
        uint8_t *value = GetValue(peers[random_peer_index].k_bucket, selected_keys[i]);

        
        printf("Key %d: ", i);
        for (int j = 0; j < 20; j++) {
            printf("%02x", selected_keys[i][j]);
        }
        printf("\nValue: ");
        if (value != NULL) {
            for (int j = 0; j < 32; j++) {
                printf("%c", value[j]);
            }
        } else {
            printf("NULL");
        }
        printf("\n\n");
    }

    return 0;
}