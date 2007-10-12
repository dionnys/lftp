/*
 * lftp and utils
 *
 * Copyright (c) 2007 by Alexander V. Lukyanov (lav@yars.free.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id$ */

#ifndef XARRAY_H
#define XARRAY_H 1

#include <sys/types.h>
#include "xmalloc.h"
#include "Ref.h"

class xarray0
{
protected:
   void *buf;
   int len;
private:
   size_t size;
   const unsigned short element_size;
   const short keep_extra;

   void init() { buf=0; size=len=0; }

   xarray0& operator=(const xarray0&); // make assignment fail
   xarray0(const xarray0&);	       // disable cloning

protected:
   void *get_ptr(int n) { return static_cast<char*>(buf)+n*element_size; }

public:
   xarray0(size_t e,int k=0) : element_size(e),keep_extra(k) { init(); }
   ~xarray0() { xfree(buf); }

   // allocates s slots, with preferred granularity g
   void get_space(size_t s,size_t g=32);
   size_t get_element_size() { return element_size; }

   int length() const { return len; }
   int count()  const { return len; }

   void _set_length(size_t n) { len=n; }
   void _nset(const void *s,int len);
   void _unset() { _nset(0,0); }
   void *_insert(int before);
   void *_append();
   void _remove(int i,int j);
   void _remove(int i) { _remove(i,i+1); }
   void _chop() { len--; }
   void *_last() { return get_ptr(len-1); }
   void *_borrow();
   void move_here(xarray0& o);

   operator bool() const { return buf!=0; }

   typedef int (*qsort_cmp_t)(const void*,const void*);
   void qsort(qsort_cmp_t cmp) {
      if(len>0)
	 ::qsort(buf,len,element_size,cmp);
   }
};

template<typename T>
class xarray : public xarray0
{
   xarray& operator=(const xarray&); // make assignment fail
   xarray(const xarray&);	       // disable cloning
public:
   xarray() : xarray0(sizeof(T)) {}
   T *get_non_const() { return static_cast<T*>(buf); }
   const T *get() const { return static_cast<const T*>(buf); }
   T& operator[](int i) { return get_non_const()[i]; }
   const T& operator[](int i) const { return get()[i]; }
   size_t get_element_size() const { return sizeof(T); }
/*   operator const T*() const { return get(); }*/
   void nset(const T *s,int len) { _nset(s,len); }
   void set(const xarray<T> &a) { nset(a.get(),a.count()); }
   void set_length(size_t n) { _set_length(n); }
   void unset() { _unset(); }
   void truncate() { set_length(0); }
   void insert(const T& n,int before) { *static_cast<T*>(_insert(before))=n; }
   void append(const T& n) { *static_cast<T*>(_append())=n; }
   void remove(int i) { _remove(i); }
   void remove(int i,int j) { _remove(i,j); }
   void chop() { _chop(); }
   T& last() { return (*this)[len-1]; }
   T *borrow() { return static_cast<T*>(_borrow()); }
   void qsort(int (*cmp)(const T*,const T*)) {
      xarray0::qsort((qsort_cmp_t)cmp);
   }
};

template<typename T,typename RefT>
class _RefArray : public xarray0
{
   void dispose(int i) { get_non_const()[i].unset(); }
   void dispose(int i,int j) { while(i<j) dispose(i++); }
   void clear(int i) { get_non_const()[i]._clear(); }
   void clear(int i,int j) { while(i<j) clear(i++); }
public:
   _RefArray() : xarray0(sizeof(RefT)) {}
   ~_RefArray() { dispose(0,len); }
   RefT *get_non_const() { return static_cast<RefT*>(buf); }
   const RefT *get() const { return static_cast<const RefT*>(buf); }
   RefT& operator[](int i) { return get_non_const()[i]; }
   const RefT& operator[](int i) const { return get()[i]; }
   size_t get_element_size() const { return sizeof(RefT); }
   void set_length(size_t n) { dispose(n,len); _set_length(n); }
   void unset() { dispose(0,len); _unset(); }
   void truncate() { set_length(0); }
   void insert(T *n,int before) { static_cast<RefT*>(_insert(before))->_set(n); }
   void append(T *n) { static_cast<RefT*>(_append())->_set(n); }
   void remove(int i) { dispose(i); _remove(i); }
   void remove(int i,int j) { dispose(i,j); _remove(i,j); }
   void chop() { dispose(len-1); _chop(); }
   RefT& last() { return (*this)[len-1]; }
   RefT *borrow() { return static_cast<RefT*>(_borrow()); }
};

template<typename T>
class RefArray : public _RefArray< T,Ref<T> > {
   RefArray& operator=(const RefArray&); // make assignment fail
   RefArray(const RefArray&);	       // disable cloning
public:
   RefArray() : _RefArray< T,Ref<T> >() {}
};

template<typename T>
class xarray_s : public _RefArray<const char,T> {
   xarray_s& operator=(const xarray_s&); // make assignment fail
   xarray_s(const xarray_s&);	       // disable cloning
public:
   xarray_s() : _RefArray<const char,T>() {}
};

template<typename T>
class xarray_p : public xarray0
{
   xarray_p& operator=(const xarray_p&); // make assignment fail
   xarray_p(const xarray_p&);	       // disable cloning

   void dispose(int i) { xfree(get_non_const()[i]); }
   void dispose(int i,int j) { while(i<j) dispose(i++); }
   void clear(int i) { get_non_const()[i]=0; }
   void clear(int i,int j) { while(i<j) clear(i++); }
   void z() { clear(len); }
public:
   xarray_p() : xarray0(sizeof(T*),1) {}
   ~xarray_p() { dispose(0,len); }
   T **get_non_const() { return static_cast<T**>(buf); }
   T *const* get() const { return static_cast<T*const*>(buf); }
   T *&operator[](int i) { return get_non_const()[i]; }
   T *operator[](int i) const { return get()[i]; }
   size_t get_element_size() const { return sizeof(T*); }
   void nset(T *const*s,int len) { dispose(0,len); _nset(s,len); if(buf) z(); }
   void set(const xarray_p<T> &a) { nset(a.get(),a.count()); }
   void set_length(size_t n) { dispose(n,len); _set_length(n); if(buf) z(); }
   void unset() { dispose(0,len); _unset(); }
   void truncate() { set_length(0); }
   void insert(T *n,int before) { *static_cast<T**>(_insert(before))=n; z(); }
   void append(T *n) { *static_cast<T**>(_append())=n; z(); }
   void remove(int i) { dispose(i); _remove(i); z(); }
   void remove(int i,int j) { dispose(i,j); _remove(i,j); z(); }
   void chop() { dispose(len-1); _chop(); }
   T *last() { return (*this)[len-1]; }
   T **borrow() { return static_cast<T**>(_borrow()); }
};


template<typename T,class A> class xqueue
{
   A q;
   int ptr;
public:
   xqueue() : ptr(0) {}
   int count() { return q.count()-ptr; }
   void empty() { q.truncate(); ptr=0; }
   void push(const T &n) {
      if(ptr>count()) {
	 q.remove(0,ptr);
	 ptr=0;
      }
      q.append(n);
   }
   // returned pointer valid till the next push
   T& next() { return q[ptr++]; }
   T& operator[](int i) { return q[ptr+i]; }
   void move_here(xqueue& o) { q.move_here(o.q); ptr=o.ptr; o.ptr=0; }
};

#endif // XARRAY_H