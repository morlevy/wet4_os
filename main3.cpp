

#include <iostream>
#include "malloc_3.cpp"

#define NUM_TESTS 10
#define MMAP_THRESHOLD 131072
#define MAX_SIZE 100000000

#define ASSERT_TRUE(expr)  		\
do {			 	 \
	if((expr) == false) {	  \
		std::cout << "Assertion Failed in " << __FILE__ << ":" << __LINE__  << std:: endl;	\
		return false;       \
	}			     \
} while(0)


#define RUN_TEST(indx) 				       \
do {							\
	std::cout << "running test" << indx << "...  ";  \
	if(tests[indx-1]()) {				  \
		std::cout << "[OK]" << std::endl;	   \
	} else {					    \
		std::cout << "[FAILED]" << std::endl; 	     \
	}						      \
} while(0)


bool isAlligned(void* addr) {
	if(addr == nullptr) { 
		return false;
	}
	unsigned long num_addr = (unsigned long)(addr);
	if(num_addr % 8 != 0) {
		return false;
	}
	return true;
}

/* basic allocation test. */
bool test1(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(1);
	void* p2 = smalloc(2);
	void* p3 = smalloc(10);
	void* p4 = smalloc(20);
	void* p5 = smalloc(30);
	void* p6 = smalloc(40);
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((6*meta_size + 128) == (d1-base) );
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(isAlligned(p2));
	ASSERT_TRUE(isAlligned(p3));
	ASSERT_TRUE(isAlligned(p4));
	ASSERT_TRUE(isAlligned(p5));
	ASSERT_TRUE(isAlligned(p6));
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 6);
	ASSERT_TRUE(_num_allocated_bytes() == 128);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	return true;
}

//basic test
bool test2(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(20);
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	ASSERT_TRUE((d1 - base) == (24 + meta_size));
	sfree(p1);
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 24);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d2 - base) == (24 + meta_size));
	return true;
}

// combine with prev
bool test3(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(10);
	void* p2 = smalloc(1);

	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(isAlligned(p2));

	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d1-base) == (2*meta_size + 24));
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 2);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	sfree(p1);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 16);
	ASSERT_TRUE(_num_allocated_blocks() == 2);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	sfree(p2);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == (24+meta_size));
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == (24+meta_size));
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d2-base) == (2*meta_size + 24));
	return true;
}

// combine with next
bool test4(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(10);
	void* p2 = smalloc(1);
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(isAlligned(p2));
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d1-base) == (2*meta_size + 24));
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 2);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	sfree(p2);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 8);
	ASSERT_TRUE(_num_allocated_blocks() == 2);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	sfree(p1);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == (24+meta_size));
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == (24+meta_size));
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d2-base) == (2*meta_size + 24));
	return true;
}

// combine with next and prev
bool test5(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(10); // 16
	void* p2 = smalloc(1); // 8
	void* p3 = smalloc(20); // 24
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(isAlligned(p2));
	ASSERT_TRUE(isAlligned(p3));
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d1-base) == (3*meta_size + 48));
	sfree(p1);
	sfree(p3);
	ASSERT_TRUE(_num_free_blocks() == 2);
	ASSERT_TRUE(_num_free_bytes() == 40);
	ASSERT_TRUE(_num_allocated_blocks() == 3);
	ASSERT_TRUE(_num_allocated_bytes() == 48);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	sfree(p2);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == (2*meta_size + 48));
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == (2*meta_size + 48));
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	return true;
}

// wilderness
bool test6(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(1);
	ASSERT_TRUE(isAlligned(p1));
	sfree(p1);
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 8) == (d1-base) );
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 8);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 8);
	p1 = smalloc(10);
	ASSERT_TRUE(isAlligned(p1));
	sfree(p1);
	p1 = smalloc(20);
	ASSERT_TRUE(isAlligned(p1));
	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 24) == (d2-base) );
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	ASSERT_TRUE(_num_meta_data_bytes() == (meta_size*_num_allocated_blocks()));
	sfree(p1);
	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 24);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 24);
	return true;
}

// maloc: get block from a free one without splitting
bool test7(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(30);
	ASSERT_TRUE(isAlligned(p1));
	sfree(p1);
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 32) == (d1-base) );

	p1 = smalloc(10);
	ASSERT_TRUE(isAlligned(p1));
	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 32) == (d2-base) );

	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 32);

	sfree(p1);

	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 32);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 32);

	p1 = smalloc(20);
	ASSERT_TRUE(isAlligned(p1));
	unsigned long d3 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 32) == (d3-base) );

	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 32);

	sfree(p1);

	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 32);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == 32);

	unsigned long d4 = (unsigned long)sbrk(0);
	ASSERT_TRUE((meta_size + 32) == (d4-base) );

	return true;
}


// maloc: get block from a free one WITH splitting
bool test8(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(300);
	void* p2 = smalloc(1);
	ASSERT_TRUE(isAlligned(p1));
	ASSERT_TRUE(isAlligned(p2));
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((2*meta_size + 312) == (d1-base) );
	sfree(p1);
	void* p3 = smalloc(176 - meta_size);
	ASSERT_TRUE(isAlligned(p3));

	unsigned long d2 = (unsigned long)sbrk(0);
	ASSERT_TRUE((2*meta_size + 312) == (d2-base) );

	ASSERT_TRUE(_num_free_blocks() == 1);
	ASSERT_TRUE(_num_free_bytes() == 128);
	ASSERT_TRUE(_num_allocated_blocks() == 3);
	ASSERT_TRUE(_num_allocated_bytes() == (312 - meta_size));

	void* p4 = smalloc(128);
	ASSERT_TRUE(isAlligned(p4));

	unsigned long d3 = (unsigned long)sbrk(0);
	ASSERT_TRUE((2*meta_size + 312) == (d3-base) );
	return true;
}

// test calloc
bool test9(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	unsigned x = MMAP_THRESHOLD-10;
	void* p1 = scalloc(x,1);
	ASSERT_TRUE(isAlligned(p1));
	for(unsigned i = 0; i < x; i++) {
		char* cur = ((char*)p1 + i);
		ASSERT_TRUE(*cur == 0);
	}
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((MMAP_THRESHOLD + meta_size - 8) == (d1-base) );
}

// mmap munmap
bool test10(void) {
	unsigned meta_size = _size_meta_data();
	unsigned long base = (unsigned long)sbrk(0);
	void* p1 = smalloc(MMAP_THRESHOLD);
	ASSERT_TRUE(isAlligned(p1));
	sfree(p1);
	ASSERT_TRUE(_num_free_blocks() == 0);
	ASSERT_TRUE(_num_free_bytes() == 0);
	ASSERT_TRUE(_num_allocated_blocks() == 1);
	ASSERT_TRUE(_num_allocated_bytes() == (MMAP_THRESHOLD + meta_size));
	unsigned long d1 = (unsigned long)sbrk(0);
	ASSERT_TRUE((d1 - base) == 0);
	return true;
}


bool (*tests[NUM_TESTS])(void) = {
	test1,
	test2,
	test3,
	test4,
	test5,
	test6,
	test7,
	test8,
	test9,
	test10
};

int main(int argc, char* argv[]) {
//	if(argc == 1) {
//        return 1;
//	} else if(argc == 2) {
//        unsigned text_idx = strtol(argv[1],nullptr,10);
//        if(text_idx >= NUM_TESTS) {
//            std::cout << "Invalid Usage - test_indx belongs to {0," << NUM_TESTS-1 << "}" << std::endl;
//            return 1;
//        }
//        RUN_TEST(text_idx + 1);
//        return 0;
//    } else {
//        std::cout << "Invalid Usage - Enter <./test_prog> in terminal to run all tests, "
//                  << "Enter <./test_prog> <test_indx> to run a specific test." << std::endl;
//        return 1;
//    }
    RUN_TEST(5);
	return 1;
}