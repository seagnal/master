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

#include <stdlib.h>
#include <cpp/memtrack.hh>
#include <cpp/debug.hh>
#include <cpp/string.hh>
#include <dlfcn.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <signal.h>

boost::detail::spinlock spinlock = BOOST_DETAIL_SPINLOCK_INIT;

CT_MEMTRACK gc_memtrack;
#if 0
void *__libc_malloc(size_t in_sz) {
	return gc_memtrack.f_malloc_libc(in_sz);
}
void __libc_free(void * in_pv) {
	return gc_memtrack.f_free_libc(in_pv);
}
#endif

#ifndef FF_MEMTRACK
void *__libc_malloc(size_t in_sz) {
	return malloc(in_sz);
}
void __libc_free(void * in_pv) {
	free(in_pv);
}
void * __libc_realloc(void * oldBuffer, size_t newSize) {
	return realloc(oldBuffer, newSize);
}
#else
// FIXME !!!! UCLIBC NG doenot have __libc_malloc
#endif
CT_MEMTRACK::CT_MEMTRACK() {
	// DONOT INIT variable , this constructor is called multiple time
	//_pf_free = NULL;
	//_pf_free =;
	//printf("INIT %p !!!\n", this);

}
CT_MEMTRACK::~CT_MEMTRACK() {
	//printf("DISABLING %p !!!\n", this);
}

void CT_MEMTRACK::f_start() {
	ST_DEBUG_BACKTRACE s_tmp;
	CT_DEBUG::f_get_backtrace(s_tmp);
	//CT_DEBUG::f_print_backtrace_cxx(s_tmp);
	b_enabled = true;
}

void CT_MEMTRACK::f_end() {
	f_display_stats(true);
	{
		CT_GUARD_LOCK c_lock(_c_lock_info);
		_m_info_register.clear();
	}
	b_enabled = false;
}

void CT_MEMTRACK::f_init() {

	LIST_INIT(&_s_memtrack_head);
	b_init = true;

#if 0
	void* handle = (void*) dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
	M_ASSERT(handle!=NULL);

	_pf_malloc = (void * (*)(int))dlsym(handle,"malloc");if
	( _pf_malloc == NULL)
	{
		printf("Error in locating\n");
		M_BUG();
	}
	_pf_free = (void (*)(void *))dlsym(handle,"free");if
	( _pf_free == NULL)
	{
		printf("Error in locating\n");
		M_BUG();
	}
	_pf_realloc = (void * (*)(void*, int))dlsym(handle,"realloc");if
	( _pf_realloc == NULL)
	{
		printf("Error in locating\n");
		M_BUG();
	}
#endif

	//_c_lock.force_init();
	//_c_lock_info.force_init();

	//spinlock = BOOST_DETAIL_SPINLOCK_INIT;

	printf("INIT !!!");

}

#ifdef FF_MEMTRACK_MMAP_ALLOC

static inline const size_t f_malloc_get_offset(void) {
	return (((sizeof(ST_MEMTRACK_INFO) + 8) / (64 * 16)) + 1) * (64 * 16);
}
static inline size_t f_malloc_allocated_space(size_t in_sz) {
	size_t i_tmp = f_malloc_get_offset();
	size_t sz_pages = (((i_tmp + in_sz + 8) >> PAGE_SHIFT) + 1) + 1;
	return sz_pages << PAGE_SHIFT;
}
static inline void * f_malloc_int(size_t in_sz) {
	void * pv_tmp = __libc_memalign(PAGE_SIZE, in_sz);
#if 0
	char * pv_tmp = (char*) mmap(NULL, in_sz,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	M_ASSERT(pv_tmp != (void* )-1);
#endif
	return pv_tmp;
}
static inline void f_malloc_protect(void * in_pv, size_t in_sz) {
	if(in_sz > 16*1024) {
	assert(mprotect(((char*)in_pv)+in_sz-PAGE_SIZE, PAGE_SIZE, PROT_READ) == 0);
	}
}
static inline void f_malloc_unprotect(void * in_pv, size_t in_sz) {
	if(in_sz > 16*1024) {
	assert(
			mprotect(((char*)in_pv)+in_sz-PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE) == 0);
	}
}
static inline void f_free_int(void * pv_tmp, size_t in_sz) {
	__libc_free(pv_tmp);
}
#else

static inline const size_t f_malloc_get_offset(void) {
	return (((sizeof(ST_MEMTRACK_INFO)+8)/(64*16))+1)*(64*16);
}

static inline size_t f_malloc_allocated_space(size_t in_sz) {
	return in_sz+ f_malloc_get_offset()+8;
}
static inline void * f_malloc_int(size_t in_sz) {
#ifdef FF_MEMTRACK
	return __libc_malloc(in_sz);
#else
	return malloc(in_sz);
#endif
}
static inline void f_malloc_protect(void * in_pv, size_t in_sz) {

}
static inline void f_malloc_unprotect(void * in_pv, size_t in_sz) {

}

static inline void f_free_int(void * pv_tmp, size_t in_sz) {
	__libc_free(pv_tmp);
}
#endif

void * CT_MEMTRACK::f_malloc(size_t in_sz, int i_offset_bt) {
#if 0
	if(!b_enabled) {
		return ::malloc(in_sz);
	}
#endif

	M_ASSERT(sizeof(ST_MEMTRACK_INFO) < C_MEMTRACK_INFO_SIZE);

	if (!b_init) {
		f_init();
	}

	size_t sz_allocated = f_malloc_allocated_space(in_sz);

	char * pv_tmp = (char*) f_malloc_int(sz_allocated);
	if (!pv_tmp) {
		return NULL;
	}

	//char * pv_tmp = (char*) ::malloc(in_sz + C_MEMTRACK_INFO_SIZE);
	ST_MEMTRACK_INFO * ps_info = (ST_MEMTRACK_INFO *) pv_tmp;
	M_ASSERT(ps_info);
	char * pv_tmp_alloc = pv_tmp + f_malloc_get_offset();

	char *pv_check = pv_tmp_alloc - sizeof(uint64_t);
	*((uint64_t*) pv_check) = 0xDEADBEAFBABEDEAD;
	char *pv_check_post = pv_tmp_alloc + in_sz;
	{
		size_t i_size = sizeof(uint64_t);
		while (i_size--) {
			*pv_check_post++ = *pv_check++;
		}
	}

	/* Store malloc info */
	ps_info->b_inplace = true;
	ps_info->sz_type = in_sz;
	ps_info->sz_allocated = sz_allocated;
	ps_info->pv_pointer = pv_tmp_alloc;
	ps_info->pv_pointer_malloc = pv_tmp;
	ps_info->str_name = "UNKNOWN";
	ps_info->i_magic_header = 0xDEADBEAFBABEDEAD;
	ps_info->i_magic_footer = 0xDEADBEAFBABEDEAD;
	ps_info->i_offset_bt = i_offset_bt;
	ps_info->b_enabled = b_enabled;
	memset(ps_info->s_bt.pv_bt, 0, C_MEMTRACK_BACKTRACE * sizeof(void*));
#ifdef LF_MEMTRACK_BACKTRACE
  #if 0
  	if (b_enabled) {
  		ps_info->s_bt.sz_bt = CT_DEBUG::f_get_backtrace(ps_info->s_bt.pv_bt,
  		C_MEMTRACK_BACKTRACE);
  	}
  #endif
#endif

	if (b_enabled) {
		f_insert(ps_info);
	}
	//if((pv_tmp_alloc==(void*)0x6388e0))
	//printf("++ %p %p %d %d\n", pv_tmp_alloc, ps_info, (int)sz_pages << PAGE_SHIFT, (int)in_sz);
	//M_ASSERT(pv_tmp_alloc!=(void*)0x6388e0);

	f_malloc_protect(ps_info->pv_pointer_malloc, ps_info->sz_allocated);
	return pv_tmp_alloc;
}
void CT_MEMTRACK::f_free(void * in_pv) {
	if (in_pv) {
		char * pv_tmp = (char*) in_pv;
		pv_tmp -= f_malloc_get_offset();
		//printf("-- %p\n", in_pv);
		//if((in_pv==(void*)0x6388e0))
		//printf("- %p %p\n", in_pv, pv_tmp);

		ST_MEMTRACK_INFO * ps_info = (ST_MEMTRACK_INFO *) pv_tmp;
		if ((ps_info->pv_pointer == in_pv)
				&& (ps_info->pv_pointer_malloc == pv_tmp)
				&& (ps_info->i_magic_header == 0xDEADBEAFBABEDEAD)
				&& (ps_info->i_magic_footer == 0xDEADBEAFBABEDEAD)) {
			//printf("-- %p %p %p %p\n", in_pv, ps_info->pv_pointer, ps_info->pv_pointer_malloc, pv_tmp);
			size_t sz_allocated = ps_info->sz_allocated;

			{
				char *pv_check_pre_buffer = ((char*) in_pv) - sizeof(uint64_t);
				assert(
						*((uint64_t* )pv_check_pre_buffer)
								== 0xDEADBEAFBABEDEAD);
				char *pv_check_post_buffer = ((char*) in_pv) + ps_info->sz_type;
				{
					size_t i_size = sizeof(uint64_t);
					while (i_size--) {
						if (*pv_check_post_buffer++ != *pv_check_pre_buffer++) {
							abort();
						}
					}
				}
			}

			if (ps_info->b_enabled) {
				f_remove(ps_info);
			}

			f_malloc_unprotect(ps_info->pv_pointer_malloc,
					ps_info->sz_allocated);

			ps_info->pv_pointer_malloc = NULL;
			ps_info->pv_pointer = NULL;

			f_free_int((void*) ps_info, sz_allocated);
		} else {
			__libc_free(in_pv);
		}
	}
}

void * CT_MEMTRACK::f_realloc(void * in_pv, size_t in_sz_new) {
	char * pv_tmp = (char*) in_pv;

	if (in_pv) {
		pv_tmp -= f_malloc_get_offset();
		ST_MEMTRACK_INFO * ps_info = (ST_MEMTRACK_INFO *) pv_tmp;
		if ((ps_info->pv_pointer == in_pv)
				&& (ps_info->pv_pointer_malloc == pv_tmp)
				&& (ps_info->i_magic_header == 0xDEADBEAFBABEDEAD)
				&& (ps_info->i_magic_footer == 0xDEADBEAFBABEDEAD)) {
			void * pv_new = f_malloc(in_sz_new, 1);

			//printf("-r %p\n", in_pv);
			if (pv_new) {
				char * pv_dst = (char*) pv_new;
				char * pv_src = (char*)ps_info->pv_pointer;
				size_t i_size = M_MIN(ps_info->sz_type, in_sz_new);
				while (i_size--) {
					*pv_dst++ = *pv_src++;
				}
			}
			size_t sz_allocated = ps_info->sz_allocated;

			{
				char *pv_check_pre_buffer = ((char*) in_pv) - sizeof(uint64_t);
				assert(
						*((uint64_t* )pv_check_pre_buffer)
								== 0xDEADBEAFBABEDEAD);
				char *pv_check_post_buffer = ((char*) in_pv) + ps_info->sz_type;
				{
					size_t i_size = sizeof(uint64_t);
					while (i_size--) {
						if (*pv_check_post_buffer++ != *pv_check_pre_buffer++) {
							abort();
						}
					}
				}
			}

			if (ps_info->b_enabled) {
				f_remove(ps_info);
			}

			f_malloc_unprotect(ps_info, sz_allocated);

			ps_info->pv_pointer_malloc = NULL;
			ps_info->pv_pointer = NULL;

			//printf("-- %p %p %d\n", in_pv, ps_info, (int)sz_allocated);
			f_free_int((void*) ps_info, sz_allocated);
			return pv_new;
		} else {
			return __libc_realloc(in_pv, in_sz_new);
		}
	}
	return f_malloc(in_sz_new);
}

void CT_MEMTRACK::f_insert(ST_MEMTRACK_INFO * in_ps_tmp) {
	/* Insert head */
	{
		spinlock.lock();
		LIST_INSERT_HEAD(&_s_memtrack_head, in_ps_tmp, element);
		spinlock.unlock();
	}
}
void CT_MEMTRACK::f_remove(ST_MEMTRACK_INFO * in_ps_tmp) {
	/* Remove from list */
	{
		spinlock.lock();
		LIST_REMOVE(in_ps_tmp, element);
		spinlock.unlock();
	}
}

void CT_MEMTRACK::f_update_name(const void * in_pv_tmp,
		const char * in_str_name) {

	bool b_bug = true;

	/* Classic memory update */
#ifdef FF_MEMTRACK_NAME
	if (b_bug) {
		spinlock.lock();
		{

			struct ST_MEMTRACK_INFO *b;
			LIST_FOREACH(b, &_s_memtrack_head, element)
			{
				if (b->pv_pointer == in_pv_tmp) {
					b_bug = false;
					b->str_name = in_str_name;
					break;
				}
			}
		}
		spinlock.unlock();
	}
#endif
	if (b_bug) {
		//_CRIT<< in_pv_tmp << " is not registered:" << in_str_name;
		//CT_DEBUG::f_print_backtrace_cxx();
	}
}

void CT_MEMTRACK::f_register(ST_MEMTRACK_INFO & in_s_info,
		const void * in_pv_tmp, size_t in_i_size, const char * in_str_name, int in_i_offset_bt) {
#ifdef FF_MEMTRACK_REGISTER

	/* Store malloc info */
	in_s_info.b_inplace = false;
	in_s_info.sz_type = in_i_size;
	in_s_info.pv_pointer = in_pv_tmp;
	in_s_info.str_name = in_str_name;
	in_s_info.sz_allocated = in_i_size;
	in_s_info.i_offset_bt = in_i_offset_bt;
	memset(in_s_info.s_bt.pv_bt, 0, C_MEMTRACK_BACKTRACE * sizeof(void*));

#ifdef LF_MEMTRACK_BACKTRACE
  if(!strcmp(in_str_name,"CT_PORT_NODE")){
    in_s_info.s_bt.sz_bt = CT_DEBUG::f_get_backtrace(in_s_info.s_bt.pv_bt,
      C_MEMTRACK_BACKTRACE);
  }
#endif

	//printf("++R %p %p %d\n", &in_s_info, in_s_info.pv_pointer, (int)in_s_info.sz_allocated);
	f_insert(&in_s_info);
#endif
}

void CT_MEMTRACK::f_unregister(ST_MEMTRACK_INFO & in_s_info) {
#ifdef FF_MEMTRACK_REGISTER
	//printf("--R %p %p %d\n", &in_s_info, in_s_info.pv_pointer, (int)in_s_info.sz_allocated);
	//CT_DEBUG::f_print_backtrace_cxx();
	{
		f_remove(&in_s_info);
	}
#endif
}

void CT_MEMTRACK::f_dump_stats(bool in_b_bt) {
#if 1
	std::cout << "DUMP STATS " << std::endl;
	std::map<char const *, ST_MEMTRACK_STAT, std::less<char const *>,
			CT_MEMTRACK_MALLOCATOR<std::pair<char const *const, ST_MEMTRACK_STAT>>> m_size;

	typedef std::map<char const *, ST_MEMTRACK_STAT>::iterator m_size_iterator_t;

	{
		spinlock.lock();
		struct ST_MEMTRACK_INFO *b;
		LIST_FOREACH(b, &_s_memtrack_head, element)
		{
//			printf("%s\n", c_tmp.str_name);;
			ST_MEMTRACK_STAT & s_data = m_size[b->str_name];
			s_data.i_nb += 1;
			s_data.i_size += b->sz_type;
			s_data.i_size_allocated += b->sz_allocated;
			s_data.pv_pointer = b->pv_pointer;
#ifdef LF_MEMTRACK_BACKTRACE
			s_data.m_elem[b->s_bt].i_nb += 1;
			s_data.m_elem[b->s_bt].i_size += b->sz_type;
			s_data.m_elem[b->s_bt].i_size_allocated += b->sz_allocated;
			s_data.m_elem[b->s_bt].pv_pointer = b->pv_pointer;
			s_data.m_elem[b->s_bt].i_offset_bt = b->i_offset_bt;
#endif
		}
		spinlock.unlock();
	}
#if 0
	{
		CT_GUARD_LOCK c_lock(_c_lock_info);
		for (_m_info_iterator_t pc_it = _m_info_register.begin();
				pc_it != _m_info_register.end(); pc_it++) {
			ST_MEMTRACK_INFO & c_tmp = pc_it->second;
			ST_MEMTRACK_STAT & s_data = m_size[c_tmp.str_name];
			s_data.i_nb += 1;
			s_data.i_size += c_tmp.sz_type;
			s_data.pv_pointer = c_tmp.pv_pointer;
#ifdef LF_MEMTRACK_BACKTRACE
			s_data.m_elem[c_tmp.s_bt].i_nb += 1;
			s_data.m_elem[c_tmp.s_bt].i_size += c_tmp.sz_type;
			s_data.m_elem[c_tmp.s_bt].pv_pointer = c_tmp.pv_pointer;
#endif
		}
	}
#endif
	void * pv_func_malloc = NULL;
	void * pv_func_register = NULL;

	std::cout << "MEMTRACK STATS " << std::endl;
	for (m_size_iterator_t pc_it = m_size.begin(); pc_it != m_size.end();
			pc_it++) {
		std::string str_tmp;
		f_string_human_readable_number(str_tmp, pc_it->second.i_size,
				E_STRING_HUMAN_READABLE_MODE_IB);
		std::string str_sz_allocated;
		f_string_human_readable_number(str_sz_allocated,
				pc_it->second.i_size_allocated,
				E_STRING_HUMAN_READABLE_MODE_IB);
		std::cout << " - " << std::setw(15) << std::dec << pc_it->second.i_nb
				<< std::setw(15) << str_tmp << std::setw(15) << str_sz_allocated
				<< std::setw(100) << CT_DEBUG::f_demangle_cxx(pc_it->first)
				<< std::setw(10) << pc_it->second.m_elem.size() << std::endl;
#ifdef LF_MEMTRACK_BACKTRACE
		if (in_b_bt) {
			typedef std::map<ST_MEMTRACK_BT, ST_MEMTRACK_STAT,
					std::less<ST_MEMTRACK_BT>,
					CT_MEMTRACK_MALLOCATOR<
							std::pair<ST_MEMTRACK_BT const, ST_MEMTRACK_STAT>>>::iterator bt_iterator_t;
			typedef std::map<void*, ST_MEMTRACK_STAT, std::less<void*>,
					CT_MEMTRACK_MALLOCATOR<std::pair<void* const, ST_MEMTRACK_STAT>>> ::iterator bt_type_iterator_t;

			std::map<void*, ST_MEMTRACK_STAT, std::less<void*>,
					CT_MEMTRACK_MALLOCATOR<std::pair<void*const , ST_MEMTRACK_STAT>>> m_elem_type;
			//if (pc_it->second.m_elem.size() < 4)
			{
				//int i_index = 0;


				//int i_index_max = pc_it->second.m_elem.size();
				for (bt_iterator_t pc_it_bt = pc_it->second.m_elem.begin();
						pc_it_bt != pc_it->second.m_elem.end(); pc_it_bt++) {
					//f_string_human_readable_number(str_tmp,
					//		pc_it_bt->second.i_size,
					//		E_STRING_HUMAN_READABLE_MODE_IB);
					//f_string_human_readable_number(str_sz_allocated, pc_it->second.i_size_allocated,
					//		E_STRING_HUMAN_READABLE_MODE_IB);
					//std::cout<< "  *"<< std::setw (15) << pc_it_bt->second.i_nb << std::setw (15) << str_tmp<< std::setw (70) << std::hex << pc_it_bt->second.pv_pointer << std::endl;
					void * pv_type = NULL;

					//printf("%s\n", str_name);

					if (pv_func_malloc == NULL) {
						std::string str_name =
								CT_DEBUG::f_print_backtrace_cxx_single(
										pc_it_bt->first.pv_bt[1], false);
						if (str_name.find(
								"CT_MEMTRACK::f_malloc(unsigned long, int)")
								!= std::string::npos) {
							pv_func_malloc = pc_it_bt->first.pv_bt[1];
						}
					}
#if 1
					if (pv_func_register == NULL) {
						if(strstr(pc_it->first, "UNKNOWN") == NULL) {
							std::string str_name =
									CT_DEBUG::f_print_backtrace_cxx_single(
											pc_it_bt->first.pv_bt[1], false);
							std::cout << str_name <<std::endl;
							if (str_name.find(
									"CT_MEMTRACK::f_register(ST_MEMTRACK_INFO&, void const*, unsigned long, char const*, int)")
									!= std::string::npos) {
								pv_func_register = pc_it_bt->first.pv_bt[1];
							}
						}
					}
#endif
					if (pv_func_register) {
						if (pv_func_register == pc_it_bt->first.pv_bt[1]) {
							pv_type = pc_it_bt->first.pv_bt[3
																	+ pc_it_bt->second.i_offset_bt];
						}
					}
					if (pv_func_malloc) {
						if (pv_func_malloc == pc_it_bt->first.pv_bt[1]) {
							pv_type = pc_it_bt->first.pv_bt[3
									+ pc_it_bt->second.i_offset_bt];
						}
					}

#if 1
					if(!strcmp(pc_it->first, "CT_PORT_NODE")) {
						std::cout<< " --------- "<< std::endl;
						CT_DEBUG::f_print_backtrace_cxx(pc_it_bt->first.pv_bt, C_MEMTRACK_BACKTRACE);
            //std::cout << " * Name: "<<str_name << std::endl;
            std::cout << " * Nb: "<< std::dec <<pc_it_bt->second.i_nb << std::endl;
            std::cout << " * Size: "<< std::dec << pc_it_bt->second.i_size << std::endl;
            std::cout << " * SizeA: "<< std::dec <<pc_it_bt->second.i_size_allocated << std::endl;
					}
#endif

#if 0
	        {
						if(strstr(pc_it->first, "UNKNOWN") != NULL) {
							std::string str_name =
									CT_DEBUG::f_print_backtrace_cxx_single(
											pv_type, false);
							if (str_name.find(
									"operator new")
									!= std::string::npos) {
              std::cout << " * Name: "<<str_name << std::endl;
              std::cout << " * Nb: "<< std::dec <<pc_it_bt->second.i_nb << std::endl;
              std::cout << " * Size: "<< std::dec << pc_it_bt->second.i_size << std::endl;
              std::cout << " * SizeA: "<< std::dec <<pc_it_bt->second.i_size_allocated << std::endl;
									CT_DEBUG::f_print_backtrace_cxx(pc_it_bt->first.pv_bt, C_MEMTRACK_BACKTRACE);
							}
						}
					}

#endif


					m_elem_type[pv_type].i_nb += pc_it_bt->second.i_nb;
					m_elem_type[pv_type].i_size += pc_it_bt->second.i_size;
					m_elem_type[pv_type].i_size_allocated +=
							pc_it_bt->second.i_size_allocated;
					m_elem_type[pv_type].pv_pointer =
							pc_it_bt->second.pv_pointer;
					m_elem_type[pv_type].m_elem[pc_it_bt->first] =
							pc_it_bt->second;
					//for (uint i = 1; i < pc_it_bt->first.sz_bt; i++) {
					//	std::cout<< "  " << std::setw (15) << " "<< std::setw (15) << " " << std::setw (70) << CT_DEBUG::f_print_backtrace_cxx_single(pc_it_bt->first.pv_bt[i], false) << std::endl;
					//}
					//break;
				}
				//std::cout<< "MEMTRACK STATS BACKTRACE"<< std::endl;
				for (bt_type_iterator_t pc_it_bt = m_elem_type.begin();
						pc_it_bt != m_elem_type.end(); pc_it_bt++) {
					f_string_human_readable_number(str_tmp,
							pc_it_bt->second.i_size,
							E_STRING_HUMAN_READABLE_MODE_IB);

					f_string_human_readable_number(str_sz_allocated,
							pc_it_bt->second.i_size_allocated,
							E_STRING_HUMAN_READABLE_MODE_IB);

					if (pc_it_bt->second.i_size > 1e6) {
						std::string str_symname =
								CT_DEBUG::f_print_backtrace_cxx_single(
										pc_it_bt->first, false);
						if (str_symname.find("(null)") != std::string::npos) {
							str_symname = "**"+
									CT_DEBUG::f_print_backtrace_cxx_single(
											pc_it_bt->first, true);
						}
						std::cout << "   " << std::setw(15) << std::dec
								<< pc_it_bt->second.i_nb << std::setw(15)
								<< str_tmp << std::setw(15) << str_sz_allocated
								<< std::setw(100) << str_symname << std::endl;
					}
				}
			}
		}
#endif
	}
	std::cout << "MEMTRACK STATS END" << std::endl;

#endif
}

void CT_MEMTRACK::f_register_malloc(ST_MEMTRACK_INFO & in_s_info,
		const void * in_pv_tmp, size_t in_i_size, const char * in_str_name, int in_i_offset_bt) {
#ifdef FF_MEMTRACK
	gc_memtrack.f_register(in_s_info, in_pv_tmp, in_i_size, in_str_name, in_i_offset_bt);
#endif
}
void CT_MEMTRACK::f_register_free(ST_MEMTRACK_INFO & in_s_info) {
#ifdef FF_MEMTRACK
	gc_memtrack.f_unregister(in_s_info);
#endif
}
void CT_MEMTRACK::f_set_name(const void * in_pv_tmp, const char * in_str_name) {
#ifdef FF_MEMTRACK
	gc_memtrack.f_update_name(in_pv_tmp, in_str_name);
#endif
}

void CT_MEMTRACK::f_display_stats(bool in_b_bt) {
#ifdef FF_MEMTRACK
	gc_memtrack.f_dump_stats(in_b_bt);
#endif
}

void * CT_MEMTRACK::malloc(size_t in_sz) {
#ifdef FF_MEMTRACK
//#if 0
	return gc_memtrack.f_malloc(in_sz);
#else
	return ::malloc(in_sz);
#endif
}

void CT_MEMTRACK::free(void * in_pv) {
#ifdef FF_MEMTRACK
//#if 0
	gc_memtrack.f_free(in_pv);
#else
	::free(in_pv);
#endif
}

#ifdef FF_MEMTRACK
#ifdef FF_MEMTRACK_ALLOC_CXX
void *operator new(size_t in_i_size) {

	void *pv_tmp = gc_memtrack.f_malloc(in_i_size);
	//CT_MEMTRACK::f_register(pv_tmp, in_i_size);
	if (pv_tmp == NULL)
		throw std::bad_alloc();
	return pv_tmp;
}

void operator delete(void *in_pv_tmp) {
	//CT_MEMTRACK::f_unregister(in_pv_tmp);
	gc_memtrack.f_free(in_pv_tmp);
}

void *operator new[](size_t in_i_size) {
	void *pv_tmp = gc_memtrack.f_malloc(in_i_size);
	//CT_MEMTRACK::f_register(pv_tmp, in_i_size);
	if (pv_tmp == NULL)
		throw std::bad_alloc();
	return pv_tmp;
}
#endif
void operator delete[](void *in_pv_tmp) {
	gc_memtrack.f_free(in_pv_tmp);
}
#ifdef FF_MEMTRACK_ALLOC_C

void *
calloc(size_t nelem, size_t elsize) {
	size_t size = nelem * elsize;
	void * allocation = gc_memtrack.f_malloc(size);

	memset(allocation, 0, size);
	//printf("+c %p %d\n", allocation, (int) elsize);
	return allocation;
}

void *
realloc(void * oldBuffer, size_t newSize) {
	//printf("+r %p %d\n", oldBuffer, (int)newSize);
	void * pv_tmp = gc_memtrack.f_realloc(oldBuffer, newSize);

	return pv_tmp;
}

void* malloc(size_t size) {
	void * pv_tmp = gc_memtrack.f_malloc(size);
	//printf("+m %p %d\n", pv_tmp, (int)size);
	return pv_tmp;
}

void free(void* ptr) {
	//printf("-- %p\n", ptr);
	gc_memtrack.f_free(ptr);
}

extern "C" void *memalign(size_t alignment, size_t size) {
	void * pv_tmp = gc_memtrack.f_malloc(size);
	// printf("+v %p\n", pv_tmp);
	return pv_tmp;
}
#endif

#endif
