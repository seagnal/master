/***********************************************************************
 ** memtrack
 ***********************************************************************
 ** Copyright (c) SEAGNAL SAS
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2.1 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 ** MA 02110-1301 USA
 **
 ** -------------------------------------------------------------------
 **
 ** As a special exception to the GNU Lesser General Public License, you
 ** may link, statically or dynamically, a "work that uses this Library"
 ** with a publicly distributed version of this Library to produce an
 ** executable file containing portions of this Library, and distribute
 ** that executable file under terms of your choice, without any of the
 ** additional requirements listed in clause 6 of the GNU Lesser General
 ** Public License.  By "a publicly distributed version of this Library",
 ** we mean either the unmodified Library as distributed by the copyright
 ** holder, or a modified version of this Library that is distributed under
 ** the conditions defined in clause 3 of the GNU Lesser General Public
 ** License.  This exception does not however invalidate any other reasons
 ** why the executable file might be covered by the GNU Lesser General
 ** Public License.
 **
 ***********************************************************************/

/* define against mutual inclusion */

#ifndef MEMTRACK_HH_
#define MEMTRACK_HH_

/**
 * @file memtrack.hh
 * Memory tracker
 *
 * @author SEAGNAL (romain.pignatari@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <typeinfo>
#include <c/common.h>
#include <pthread.h>
#include <cpp/lock.hh>
#include <map>
#include <list>
#include <sys/queue.h>

//#define FF_MEMTRACK
#define FF_MEMTRACK_ALLOC_CXX
#define FF_MEMTRACK_ALLOC_C

//#define FF_MEMTRACK_MMAP_ALLOC

#define LF_MEMTRACK_BACKTRACE
#define FF_MEMTRACK_NAME_PORT_NODE
#define FF_MEMTRACK_REGISTER
#define FF_MEMTRACK_DISPLAY_IDLE
//#define FF_MEMTRACK_REGISTER_NAME

extern "C" void *__libc_malloc(size_t);
extern "C" void __libc_free(void *);
extern "C" void * __libc_realloc(void *, size_t);
extern "C" void * __libc_memalign(size_t, size_t);

template <class T>
struct CT_MEMTRACK_MALLOCATOR : public std::allocator<T> {
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template <class U> struct rebind { typedef CT_MEMTRACK_MALLOCATOR<U> other; };
    CT_MEMTRACK_MALLOCATOR() throw() {}
    CT_MEMTRACK_MALLOCATOR(const CT_MEMTRACK_MALLOCATOR&) throw() {}

    template <class U> CT_MEMTRACK_MALLOCATOR(const CT_MEMTRACK_MALLOCATOR<U>&) throw(){}

    ~CT_MEMTRACK_MALLOCATOR() throw() {}

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    pointer allocate(size_type s, void const * = 0) {
        if (0 == s)
            return NULL;
        pointer temp = (pointer) __libc_malloc(s * sizeof(T));
        if (temp == NULL)
            throw std::bad_alloc();
        return temp;
    }

    void deallocate(pointer p, size_type) {
    	__libc_free(p);
    }

    size_type max_size() const throw() {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    void construct(pointer p, const T& val) {
        new((void *)p) T(val);
    }

    void destroy(pointer p) {
        p->~T();
    }
};
#if 0
template<typename T>
class CT_MEMTRACK_MALLOCATOR: public std::allocator<T> {
public:
	typedef size_t size_type;
	typedef T* pointer;
	typedef const T* const_pointer;

	template<typename _Tp1>
	struct rebind {
		typedef CT_MEMTRACK_MALLOCATOR<_Tp1> other;
	};

	pointer allocate(size_type n, const void *hint = 0) {
		//  fprintf(stderr, "Alloc %d bytes. %p\n", (int)(n*sizeof(T)), hint);
		pointer pc_tmp = (pointer) malloc((int) (n * sizeof(T)));
		M_ASSERT(pc_tmp);
		//pointer new_memory = reinterpret_cast<pointer>(::operator new(pc_tmp)(n * sizeof (T)));
		return pc_tmp;
	}

	void deallocate(pointer p, size_type n) {
		// fprintf(stderr, "Dealloc %d bytes (%p).\n", n*sizeof(T), p);
		//free(p);
		//if(p) {
		free(p);
		//}
	}

	CT_MEMTRACK_MALLOCATOR() throw () :
			std::allocator<T>() {
	}
	CT_MEMTRACK_MALLOCATOR(const CT_MEMTRACK_MALLOCATOR &a) throw () :
			std::allocator<T>(a) {
	}
	template<class U>
	CT_MEMTRACK_MALLOCATOR(const CT_MEMTRACK_MALLOCATOR<U> &a) throw () :
			std::allocator<T>(a) {
	}
	~CT_MEMTRACK_MALLOCATOR() throw () {
	}
};
#endif
#define C_MEMTRACK_BACKTRACE 15
#define C_MEMTRACK_INFO_SIZE (64*16)
struct ST_MEMTRACK_BT {
	void * pv_bt[C_MEMTRACK_BACKTRACE];
	size_t sz_bt;

	bool operator <(const ST_MEMTRACK_BT& rhs) const {

		if (sz_bt != rhs.sz_bt) {
			return sz_bt < rhs.sz_bt;
		}

		for (uint i = 0; i < sz_bt; i++) {
			if (pv_bt[i] != rhs.pv_bt[i]) {
				return pv_bt[i] < rhs.pv_bt[i];
			}
		}
		return false;
	}

	ST_MEMTRACK_BT() {
		sz_bt = 0;
	}
};
struct ST_MEMTRACK_INFO {
	uint64_t i_magic_header;
	void const * pv_pointer;
	void * pv_pointer_malloc;
	size_t sz_type;
	size_t sz_allocated;
	int i_offset_bt;
	const char * str_name;
	bool b_inplace;
	bool b_enabled;

#ifdef LF_MEMTRACK_BACKTRACE
	ST_MEMTRACK_BT s_bt;
#endif

	LIST_ENTRY(ST_MEMTRACK_INFO)
	element;
	uint64_t i_magic_footer;
};

struct ST_MEMTRACK_STAT {
	uint64_t i_nb;
	uint64_t i_size;
	uint64_t i_size_allocated;
	uint64_t i_offset_bt;
	void const * pv_pointer;

#ifdef LF_MEMTRACK_BACKTRACE
	std::map<ST_MEMTRACK_BT, ST_MEMTRACK_STAT, std::less<ST_MEMTRACK_BT>,
			CT_MEMTRACK_MALLOCATOR<std::pair<const ST_MEMTRACK_BT, ST_MEMTRACK_STAT>>> m_elem;
#endif

	ST_MEMTRACK_STAT() {
		i_nb = 0;
		i_size = 0;
		i_size_allocated = 0;
		i_offset_bt = 0;
		pv_pointer = NULL;
	}
};

class CT_MEMTRACK;
class CT_MEMTRACK {
	//static volatile __thread bool b_in_process;
	//static std::map<const void *, ST_MEMTRACK_INFO> gm_info;
	//typedef std::map<const void *, ST_MEMTRACK_INFO>::iterator gm_info_iterator_t;
	//static pthread_mutex_t lock;
	//static boost::detail::spinlock lock;
	//static __thread boost::detail::spinlock lock_thread;
	//static __thread bool lock_thread_init;
	//static bool b_enabled;
	CT_SPINLOCK _c_lock;

	//static CT_MEMTRACK gc_memtrack;
	typedef LIST_HEAD(,ST_MEMTRACK_INFO)
	_s_memtrack_head_t;
	_s_memtrack_head_t _s_memtrack_head;

	CT_SPINLOCK _c_lock_info;
	std::map<const void *, ST_MEMTRACK_INFO, std::less<const void *>,
			CT_MEMTRACK_MALLOCATOR<std::pair<const void * const, ST_MEMTRACK_INFO>>> _m_info_register;
	typedef std::map<const void *, ST_MEMTRACK_INFO>::iterator _m_info_iterator_t;
#if 1
	void* (*_pf_malloc)(int);
	void (*_pf_free)(void*);
	void* (*_pf_realloc)(void*,int);
#endif
	bool b_init;
	bool b_enabled;
public:
	CT_MEMTRACK();
	~CT_MEMTRACK();

	void f_init();
	void f_start();
	void f_end();

	void * f_malloc(size_t in_sz, int i_offset_bt = 0);
	void f_free(void * in_pv);
	void *  f_realloc(void * in_pv, size_t in_sz_new);

	void f_insert(ST_MEMTRACK_INFO * in_ps_tmp);
	void f_remove(ST_MEMTRACK_INFO * in_ps_tmp);
	void f_update_name(const void * in_pv_tmp, const char * in_str_name);

	void f_register(ST_MEMTRACK_INFO & in_s_info, const void * in_pv_tmp, size_t in_i_size, const char * in_str_name, int in_i_offset_bt);
	void f_unregister(ST_MEMTRACK_INFO & in_s_info);

	void f_dump_stats(bool in_b_bt = false);

	void * f_malloc_libc(size_t in_sz);
	void f_free_libc(void * in_pv);

	static void f_register_malloc(ST_MEMTRACK_INFO & in_s_info, const void * in_pv_tmp, size_t in_i_size, const char * in_str_name, int in_i_offset_bt = 0);
	static void f_register_free(ST_MEMTRACK_INFO & in_s_info);
	static void f_set_name(const void * in_pv_tmp, const char * in_str_name);

	static void f_display_stats(bool in_b_bt = false);

	static void * malloc(size_t in_sz);
	static void free(void * in_pv);

	_s_memtrack_head_t & f_get_head() {return _s_memtrack_head;}
};

extern CT_MEMTRACK gc_memtrack;

#endif /* MEMTRACK_HH_ */
