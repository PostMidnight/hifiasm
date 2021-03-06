#ifndef __HASHTABLE__
#define __HASHTABLE__
#include "khash.h"
#include "kmer.h"

KHASH_MAP_INIT_INT64(COUNT64, int) 
typedef khash_t(COUNT64) Count_Table;

KHASH_MAP_INIT_INT64(POS64, uint64_t) 
typedef khash_t(POS64) Pos_Table;

#define PREFIX_BITS 16
#define MAX_SUFFIX_BITS 64
#define MODE_VALUE 101

///#define WINDOW 350
///#define THRESHOLD  14

#define WINDOW 375
//#define WINDOW_BOUNDARY 150
#define WINDOW_BOUNDARY 375
///for one side, the first or last WINDOW_UNCORRECT_SINGLE_SIDE_BOUNDARY bases should not be corrected
#define WINDOW_UNCORRECT_SINGLE_SIDE_BOUNDARY 25
#define THRESHOLD  15
#define THRESHOLD_RATE 0.04
#define TAIL_LENGTH int(1/THRESHOLD_RATE)
///#define OVERLAP_THRESHOLD 0.9
#define OVERLAP_THRESHOLD_FILTER 0.9
#define WINDOW_MAX_SIZE WINDOW + TAIL_LENGTH + 3
#define THRESHOLD_MAX_SIZE  31
#define FINAL_OVERLAP_ERROR_RATE 0.03

#define GROUP_SIZE 4
///the max cigar likes 10M10D10M10D10M
///#define CIGAR_MAX_LENGTH THRESHOLD*2+2
#define CIGAR_MAX_LENGTH 31*2+4

typedef struct
{
    volatile int lock;

}Hash_table_spin_lock;

typedef struct
{
	Count_Table** sub_h;
    Hash_table_spin_lock* sub_h_lock;
    int prefix_bits;
    int suffix_bits;
    ///number of subtable
	int size;
    uint64_t suffix_mode;
    uint64_t non_unique_k_mer;
} Total_Count_Table;

typedef struct
{
    uint64_t offset;
    uint64_t readID;
} k_mer_pos;

typedef struct
{
    k_mer_pos* list;
    uint64_t length;
    uint64_t size;
    uint8_t direction;
    uint64_t end_pos;
} k_mer_pos_list;

typedef struct
{
    k_mer_pos_list* list;
    uint64_t size;
    uint64_t length;
} k_mer_pos_list_alloc;


typedef struct
{
  int C_L[CIGAR_MAX_LENGTH];
  char C_C[CIGAR_MAX_LENGTH];
  int length;
} CIGAR;

typedef struct
{
  ///the begining and end of a window, instead of the whole overlap
  uint64_t x_start;
  uint64_t x_end;
  int y_end;
  int y_start;
  int extra_begin;
  int extra_end;
  int error_threshold;
  int error;
  CIGAR cigar;
} window_list;


typedef struct
{
    window_list* buffer;
    long long length;
    long long size;
}window_list_alloc;


typedef struct
{
    uint64_t* buffer;
    uint64_t length;
    uint64_t size;
}Fake_Cigar;

typedef struct
{
    uint64_t x_id;
    ///the begining and end of the whole overlap
    uint64_t x_pos_s;
    uint64_t x_pos_e;
    uint64_t x_pos_strand;

    uint64_t y_id;
    uint64_t y_pos_s;
    uint64_t y_pos_e;
    uint64_t y_pos_strand;

    uint64_t overlapLen;
    uint64_t shared_seed;
    uint64_t align_length;
    ///uint64_t total_errors;
    uint8_t is_match;
    uint8_t without_large_indel;
    uint64_t non_homopolymer_errors;

    window_list* w_list;
    uint64_t w_list_size;
    uint64_t w_list_length;
    int8_t strong;
    Fake_Cigar f_cigar;

    window_list_alloc boundary_cigars;
} overlap_region;


typedef struct
{
    overlap_region* list;
    uint64_t size;
    uint64_t length;
    ///uint64_t mapped_overlaps_length;
    long long mapped_overlaps_length;
} overlap_region_alloc;

typedef struct
{
    ///uint64_t offset;
    long long offset;
    ///uint64_t self_offset;
    long long self_offset;
    uint64_t readID;
    uint8_t strand;
} k_mer_hit;


typedef struct
{
  k_mer_hit node;
  uint64_t ID;
} ElemType;


typedef struct 
{
    ElemType* heap; 
    uint64_t* index_i;
    int len; 
    int MaxSize;    
} HeapSq;

typedef struct
{
    long long* score;
    long long* pre;
    long long* indels;
    long long* self_length;
    long long length;
    long long size;
} Chain_Data;

typedef struct
{
    k_mer_hit* list;
    k_mer_hit* tmp;
    long long length;
    long long size;
    uint64_t foward_pos;
    uint64_t rc_pos;
    Chain_Data chainDP;
} Candidates_list;

typedef struct
{
	Pos_Table** sub_h;
    Hash_table_spin_lock* sub_h_lock;
    int prefix_bits;
    int suffix_bits;
    ///number of subtable
	int size;
    uint64_t suffix_mode;
    k_mer_pos* pos;
    uint64_t useful_k_mer;
    uint64_t total_occ;
    uint64_t* k_mer_index;
} Total_Pos_Table;




inline uint64_t mod_d(uint64_t h_key, uint64_t low_key, uint64_t d)
{
    uint64_t result = (h_key >> 32) % d;
    result = ((result << 32) + (h_key & (uint64_t)0xffffffff)) % d;
    result = ((result << 32) + (low_key >> 32)) % d;
    result = ((result << 32) + (low_key & (uint64_t)0xffffffff)) % d;

    return result;
}



////suffix_bits = 64 in default
inline int recover_hash_code(uint64_t sub_ID, uint64_t sub_key, Hash_code* code,
uint64_t suffix_mode, int suffix_bits, int k)
{
    uint64_t h_key, low_key;
    h_key = low_key = 0;

    low_key = sub_ID << SAFE_SHIFT(suffix_bits);
    low_key = low_key | sub_key;

    h_key = sub_ID >> (64 - suffix_bits);

    code->x[0] = code->x[1] = 0;
    uint64_t mask = ALL >> (64 - k);
    code->x[0] = low_key & mask;

    code->x[1] = h_key << (64 - k);
    code->x[1] = code->x[1] | (low_key >> SAFE_SHIFT(k));

    return 1;
}

///inline int get_sub_table(uint64_t* get_sub_ID, uint64_t* get_sub_key, Total_Count_Table* TCB, Hash_code* code, int k)
inline int get_sub_table(uint64_t* get_sub_ID, uint64_t* get_sub_key, uint64_t suffix_mode, int suffix_bits,
Hash_code* code, int k)
{
    uint64_t h_key, low_key;
    ///k might be 64，so it is unsafe
    ///low_key = code->x[0] | (code->x[1] << k);
    low_key = code->x[0] | (code->x[1] << SAFE_SHIFT(k));
    //k cannot be 0, so this shift is safe
    h_key = code->x[1] >> (64 - k);

    if(mod_d(h_key, low_key, MODE_VALUE) > 3)
    {
        return 0;
    }

    uint64_t sub_ID = (low_key >> SAFE_SHIFT(suffix_bits)) | (h_key << (64 - suffix_bits));
    uint64_t sub_key = (low_key & suffix_mode);

    *get_sub_ID = sub_ID;
    *get_sub_key = sub_key;

    return 1;
}


inline int insert_Total_Count_Table(Total_Count_Table* TCB, Hash_code* code, int k)
{
    uint64_t sub_ID, sub_key;
    if(!get_sub_table(&sub_ID, &sub_key, TCB->suffix_mode, TCB->suffix_bits, code, k))
    {
        return 0;
    }

    khint_t t;  
    int absent;


    while (__sync_lock_test_and_set(&TCB->sub_h_lock[sub_ID].lock, 1))
    {
        while (TCB->sub_h_lock[sub_ID].lock);
    }

    t = kh_put(COUNT64, TCB->sub_h[sub_ID], sub_key, &absent);
    if (absent)
    {
        kh_value(TCB->sub_h[sub_ID], t) = 1;
    }
    else   
    {
        //kh_value(TCB->sub_h[sub_ID], t) = kh_value(TCB->sub_h[sub_ID], t) + 1;
        kh_value(TCB->sub_h[sub_ID], t)++;
    }

    __sync_lock_release(&TCB->sub_h_lock[sub_ID].lock);

    return 1;
}

inline int get_Total_Count_Table(Total_Count_Table* TCB, Hash_code* code, int k)
{

    uint64_t sub_ID, sub_key;
    if(!get_sub_table(&sub_ID, &sub_key, TCB->suffix_mode, TCB->suffix_bits, code, k))
    {
        return 0;
    }

    khint_t t;  

    ///query hash table，key is k
    t = kh_get(COUNT64, TCB->sub_h[sub_ID], sub_key);

    if (t != kh_end(TCB->sub_h[sub_ID]))
    {
        return kh_value(TCB->sub_h[sub_ID], t);
    }
    else
    {
        return 0;
    }


}



inline uint64_t get_Total_Pos_Table(Total_Pos_Table* PCB, Hash_code* code, int k, uint64_t* r_sub_ID)
{

    uint64_t sub_ID, sub_key;
    if(!get_sub_table(&sub_ID, &sub_key, PCB->suffix_mode, PCB->suffix_bits, code, k))
    {
        return (uint64_t)-1;
    }

    khint_t t;  

    ///query hash table，key is k
    t = kh_get(POS64, PCB->sub_h[sub_ID], sub_key);

    if (t != kh_end(PCB->sub_h[sub_ID]))
    {
        *r_sub_ID = sub_ID;
        return kh_value(PCB->sub_h[sub_ID], t);
    }
    else
    {
        return (uint64_t)-1;
    }


}

inline uint64_t count_Total_Pos_Table(Total_Pos_Table* PCB, Hash_code* code, int k)
{
    uint64_t sub_ID;
    uint64_t ret = get_Total_Pos_Table(PCB, code, k, &sub_ID);
    if(ret != (uint64_t)-1)
    {
        return PCB->k_mer_index[ret + 1] - PCB->k_mer_index[ret];
    }
    else
    {
        return 0;
    }
}


inline uint64_t locate_Total_Pos_Table(Total_Pos_Table* PCB, Hash_code* code, k_mer_pos** list, int k, uint64_t* r_sub_ID)
{
    uint64_t ret = get_Total_Pos_Table(PCB, code, k, r_sub_ID);
    if(ret != (uint64_t)-1)
    {
        *list = PCB->k_mer_index[ret] + PCB->pos;
        return PCB->k_mer_index[ret + 1] - PCB->k_mer_index[ret];
    }
    else
    {
        *list = NULL;
        return 0;
    }
}

int cmp_k_mer_pos(const void * a, const void * b);


inline uint64_t insert_Total_Pos_Table(Total_Pos_Table* PCB, Hash_code* code, int k, uint64_t readID, uint64_t pos)
{
    k_mer_pos* list;
    int flag = 0;
    uint64_t sub_ID;
    uint64_t occ = locate_Total_Pos_Table(PCB, code, &list, k, &sub_ID);

    if (occ)
    {   

        while (__sync_lock_test_and_set(&PCB->sub_h_lock[sub_ID].lock, 1))
        {
            while (PCB->sub_h_lock[sub_ID].lock);
        }

        if (list[0].offset + 1 < occ)
        {
            list[0].offset++;
            list[list[0].offset].readID = readID;
            ///list[list[0].offset].readID = readID|direction;
            list[list[0].offset].offset = pos;
        }
        else
        {
            list[0].readID = readID;
            ///list[0].readID = readID|direction;
            list[0].offset = pos;
            flag = 1;
        }

        __sync_lock_release(&PCB->sub_h_lock[sub_ID].lock);

        //if all pos has been saved, it is safe to sort
        if (flag && occ>1)
        {
            qsort(list, occ, sizeof(k_mer_pos), cmp_k_mer_pos);
        }

        
        return 1;
    }
    else
    {
        return 0;
    }
    
    
}


void init_Total_Count_Table(int k, Total_Count_Table* TCB);
void init_Total_Pos_Table(Total_Pos_Table* TCB, Total_Count_Table* pre_TCB);
void destory_Total_Count_Table(Total_Count_Table* TCB);

void init_Count_Table(Count_Table** table);
void init_Pos_Table(Count_Table** pre_table, Pos_Table** table);
void destory_Total_Pos_Table(Total_Pos_Table* TCB);
void write_Total_Pos_Table(Total_Pos_Table* TCB, char* read_file_name);
int load_Total_Pos_Table(Total_Pos_Table* TCB, char* read_file_name);



void Traverse_Counting_Table(Total_Count_Table* TCB, Total_Pos_Table* PCB, int k_mer_min_freq, int k_mer_max_freq);

void init_Candidates_list(Candidates_list* l);
void clear_Candidates_list(Candidates_list* l);
void destory_Candidates_list(Candidates_list* l);


void init_k_mer_pos_list_alloc(k_mer_pos_list_alloc* list);
void destory_k_mer_pos_list_alloc(k_mer_pos_list_alloc* list);
void clear_k_mer_pos_list_alloc(k_mer_pos_list_alloc* list);
void append_k_mer_pos_list_alloc(k_mer_pos_list_alloc* list, k_mer_pos* n_list, uint64_t n_length, 
uint64_t n_end_pos, uint8_t n_direction);



void merge_k_mer_pos_list_alloc_heap_sort(k_mer_pos_list_alloc* list, Candidates_list* candidates, HeapSq* HBT);

void Init_Heap(HeapSq* HBT);
void destory_Heap(HeapSq* HBT);
void clear_Heap(HeapSq* HBT);

void init_overlap_region_alloc(overlap_region_alloc* list);
void clear_overlap_region_alloc(overlap_region_alloc* list);
void destory_overlap_region_alloc(overlap_region_alloc* list);
void append_window_list(overlap_region* region, uint64_t x_start, uint64_t x_end, int y_start, int y_end, int error,
int extra_begin, int extra_end, int error_threshold);



void overlap_region_sort_y_id(overlap_region *a, long long n);


void calculate_overlap_region_by_chaining(Candidates_list* candidates, overlap_region_alloc* overlap_list, 
uint64_t readID, uint64_t readLength, All_reads* R_INF, double band_width_threshold, int add_beg_end);



void init_fake_cigar(Fake_Cigar* x);
void destory_fake_cigar(Fake_Cigar* x);
void clear_fake_cigar(Fake_Cigar* x);
void add_fake_cigar(Fake_Cigar* x, uint32_t gap_site, int32_t gap_shift);
void resize_fake_cigar(Fake_Cigar* x, uint64_t size);
int get_fake_gap_pos(Fake_Cigar* x, int index);
int get_fake_gap_shift(Fake_Cigar* x, int index);
inline long long y_start_offset(long long x_start, Fake_Cigar* o)
{
    if(x_start == get_fake_gap_pos(o, o->length - 1))
    {
        return get_fake_gap_shift(o, o->length - 1);
    }
    
    
    long long i;
    for (i = 0; i < (long long)o->length; i++)
    {
        if(x_start < get_fake_gap_pos(o, i))
        {
            break;
        }
    }

    if(i == 0 || i == (long long)o->length)
    {
        fprintf(stderr, "ERROR\n");
        exit(0);
    }

    ///note here return i - 1
    return get_fake_gap_shift(o, i - 1);
}

inline void print_fake_gap(Fake_Cigar* o)
{
    long long i;
    for (i = 0; i < (long long)o->length; i++)
    {
        fprintf(stderr, "**i: %lld, gap_pos_in_x: %d, gap_shift: %d\n", 
           i, get_fake_gap_pos(o, i),
           get_fake_gap_shift(o, i));
    }

}

void resize_Chain_Data(Chain_Data* x, long long size);
void init_window_list_alloc(window_list_alloc* x);
void clear_window_list_alloc(window_list_alloc* x);
void destory_window_list_alloc(window_list_alloc* x);
void resize_window_list_alloc(window_list_alloc* x, long long size);


#endif