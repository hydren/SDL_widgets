/*  
    Demo program for SDL-widgets-1.0
    Copyright 2011-2013 W.Boeke

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.
*/

template<class T,Uint32 dim>
struct Array {  // can insert and remove
  int lst;
  T buf[dim];
  Array():lst(-1) {
  }
  T& operator[](Uint32 ind) {
    if (ind<dim) {
      if (lst<int(ind)) lst=ind;
      return buf[ind];
    }
    alert("Array: index=%d (>=%d)",ind,dim);// if (debug) abort();
    return buf[0];
  }
  T* operator+(Uint32 ind) {
    if (ind<dim) {
      if (lst<int(ind)) lst=ind;
      return buf+ind;
    }
    alert("Array: index=%d (>=%d)",ind,dim);// if (debug) abort();
    return buf;
  }
  void insert(int ind,T item) {
    if (lst>=int(dim-1)) { alert("Array insert: lst=%d (>=%d-1)",lst,dim); return; }
    ++lst;
    for (int j=lst;j>ind;--j) buf[j]=buf[j-1];
    buf[ind]=item;
  }
  void remove(int ind) {
    if (lst<0) return;
    for (int j=ind;j<lst;++j) buf[j]=buf[j+1];
    --lst;
  }
};
