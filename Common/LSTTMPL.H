// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace WorkbenchLib
{

#define PTR_SET_DEFINED

class COMPARE_FUNCTION
{
public:
	inline COMPARE_FUNCTION ()
	{
		Compare = 0;
	}

	int (*Compare)(void *obj0,void *obj1);
};

#define ASSIGN_CHANGES_MADE_IMPLEMENTATION(PTR_CLASS)\
	inline BOOL Assign (const PTR_CLASS &obj, BOOL &bChangesMade)\
	{\
		int32_t sz0 = GetStorageSizeBytes ();\
		int32_t sz1 = obj.GetStorageSizeBytes ();\
\
		unsigned char *b1 = new unsigned char[sz1];\
		if (!b1) return FALSE;\
\
		int32_t check = obj.SaveToBuffer (b1);\
		if (check != sz1) return FALSE;\
\
		if (sz0 != sz1)\
			bChangesMade = TRUE;\
		else\
		{\
			unsigned char *b0 = new unsigned char[sz0];\
			if (!b0) return FALSE;\
			check = SaveToBuffer (b0);\
			if (check != sz0) return FALSE;\
\
			if (BuffersAreIdentical (b0,b1,sz0) == FALSE)\
				bChangesMade = TRUE;\
\
			delete [] b0;\
		}\
\
		check = LoadFromBuffer (b1);\
		delete [] b1;\
\
		if (check != sz1) return FALSE;\
\
		return TRUE;\
	}

#define ASSIGN_IMPLEMENTATION(PTR_CLASS)\
	inline BOOL Assign (const PTR_CLASS &obj)\
	{\
		int32_t sz1 = obj.GetStorageSizeBytes ();\
\
		unsigned char *b1 = new unsigned char[sz1];\
		if (!b1) return FALSE;\
\
		int32_t check = obj.SaveToBuffer (b1);\
		if (check != sz1) return FALSE;\
\
		check = LoadFromBuffer (b1);\
		delete [] b1;\
\
		if (check != sz1) return FALSE;\
\
		return TRUE;\
	}


template <class PTR_CLASS, class INDEX_TYPE=uint32_t> class PtrSet 
{
private:

	INDEX_TYPE m_alloc_increment_size;
	INDEX_TYPE m_alloc_n;
	INDEX_TYPE m_ptrs_n;
	PTR_CLASS **m_ptrs;

	INDEX_TYPE *m_idx;

	PTR_CLASS *m_block;
	INDEX_TYPE m_block_sz;

	INDEX_TYPE m_num_alt_indexes;
	INDEX_TYPE **m_alt_indexes;
	COMPARE_FUNCTION *m_alt_indexes_compare_funcs;

	union
	{
		BYTE m_status;
		struct 
		{
			BYTE m_delete_objects		 : 1;
			BYTE m_create_linear_idx     : 1;
			BYTE m_reserved0			 : 1;
			BYTE m_unique		         : 1;
			BYTE m_memory_block_mode	 : 1;
			BYTE m_reserved		         : 3;
		};
	};

public:
	
	inline PtrSet (INDEX_TYPE alloc_increment_size)
	{
		m_status = 0;
		m_alloc_increment_size = alloc_increment_size;
		m_alloc_n = 0;
		m_ptrs_n = 0;
		m_ptrs = 0;
		m_idx = 0;
		m_delete_objects = FALSE;
		m_create_linear_idx = TRUE;
		m_unique = FALSE;
		m_memory_block_mode = FALSE;
		m_block = 0;
		m_block_sz = 0;

		m_num_alt_indexes = 0;
		m_alt_indexes = 0;
		m_alt_indexes_compare_funcs = 0;

	}

	inline PTR_CLASS& operator[](INDEX_TYPE idx)
	{
		PTR_CLASS* obj = GetObject(idx);
		return *obj;
	}

	inline INDEX_TYPE size(void) const
	{
		return GetCount();
	}

	inline void DoNotDeleteObjects (void)
	{
		m_delete_objects = FALSE;
	}

	inline PtrSet (void)
	{
		m_status = 0;
		m_alloc_increment_size = 0;
		m_alloc_n = 0;
		m_ptrs_n = 0;
		m_ptrs = 0;
		m_idx = 0;
		m_delete_objects = TRUE;
		m_create_linear_idx = TRUE;
		m_unique = FALSE;
		m_memory_block_mode = FALSE;
		m_block = 0;
		m_block_sz = 0;

		m_num_alt_indexes = 0;
		m_alt_indexes = 0;
		m_alt_indexes_compare_funcs = 0;
	}

	inline ~PtrSet (void)
	{
		delete_pointers (TRUE);
		m_ptrs_n = -1;
	}

	inline BOOL SetNumberOfAlternateIndexes (INDEX_TYPE n)
	{
		ASSERT(m_num_alt_indexes == 0);

		m_alt_indexes = new INDEX_TYPE*[n];
		ASSERT(m_alt_indexes);

		memset(m_alt_indexes,0,sizeof(INDEX_TYPE*)*n);
		m_num_alt_indexes = n;

		m_alt_indexes_compare_funcs = new COMPARE_FUNCTION[n];
		ASSERT(m_alt_indexes_compare_funcs);

		memset(m_alt_indexes,0,sizeof(COMPARE_FUNCTION)*n);
		return TRUE;
	}

	inline BOOL SetAlternateIndexCompareFunction (INDEX_TYPE i, int(*Compare)(void*,void*))
	{
		if (i < 0 || i >= m_num_alt_indexes)
		{
			ASSERT(FALSE);
			return FALSE;
		}

		ASSERT(m_alt_indexes_compare_funcs[i].Compare == 0);
	
		m_alt_indexes_compare_funcs[i].Compare = (int(*)(void*,void*))Compare;
		return TRUE;
	}

	inline INDEX_TYPE GetAlternateRawIndexDirect (INDEX_TYPE i_alt_index, INDEX_TYPE index)
	{
		if (i_alt_index < 0 || i_alt_index >= m_num_alt_indexes)
		{
			ASSERT(FALSE);
			return 0;
		}

		if (index < 0 || index >= m_ptrs_n)
		{
			ASSERT(FALSE);
			return 0;
		}

		return m_alt_indexes[i_alt_index][index];
	}

	inline BOOL AssignAsOneBlock (PTR_CLASS *array, INDEX_TYPE N)
	{
		RemoveAll ();

		m_create_linear_idx = FALSE;
		m_delete_objects = FALSE;
		m_memory_block_mode = TRUE;

		m_block = array;
		m_block_sz = N;

		return TRUE;
	}


	inline BOOL IsValid (void)
	{
		if (m_ptrs == 0)
		{
			if (m_ptrs_n || m_alloc_n)
				return FALSE;
		}
		else
		{
			if (AfxIsValidAddress (m_ptrs, sizeof(m_ptrs[0])*m_alloc_n) == FALSE) 
				return FALSE;
		}

		if (m_create_linear_idx && (m_alloc_n || m_ptrs_n))
		{
			if (AfxIsValidAddress (m_idx, sizeof(m_idx[0])*m_alloc_n) == FALSE)
				return FALSE;
		}

		if (m_alloc_n < m_ptrs_n) return FALSE;
		if (m_alloc_increment_size < 0) return FALSE;

		if (m_block)
		{
			if (AfxIsValidAddress (m_block, sizeof(PTR_CLASS)*m_block_sz) == FALSE)
				return FALSE;
		}

		return TRUE;
	}

	inline void SetStatusDeleteObjects (BOOL status)
	{
		m_delete_objects = status;
	}

	inline BOOL GetStatusDeleteObjects (void)
	{
		return m_delete_objects;
	}

	inline void SetStatusCreateLinearIndex (BOOL status)
	{
		ASSERT (m_alloc_n == 0);

		if (status == FALSE)
			m_create_linear_idx = FALSE;
		else
			m_create_linear_idx = TRUE;
	}

	inline BOOL GetStatusContainsLinearIndex (void)
	{
		return m_create_linear_idx;
	}

	inline void SetStatusUnique (BOOL status)
	{
		m_unique = status;
		ASSERT (m_alloc_n == 0);
	}

    inline INDEX_TYPE GetAllocSizeItems (void)
    {
        return m_alloc_n;
    }

	inline BOOL increase_allocation_if_necessary (INDEX_TYPE n)
	{
		ASSERT (m_block == 0);

		if (m_alloc_n <= n)
		{
			INDEX_TYPE incr = m_alloc_increment_size;
			if (!incr) incr = m_alloc_n;
			if (!incr) incr++;
			if (m_alloc_n + incr < n) incr = n - m_alloc_n;

			PTR_CLASS **tmp = new PTR_CLASS*[incr+m_alloc_n];
			ASSERT (tmp);
			if (!tmp) FALSE;

			memmove (tmp,m_ptrs,sizeof(PTR_CLASS*)*m_ptrs_n);
			memset (&tmp[m_alloc_n],0,sizeof(PTR_CLASS*)*incr);

			delete m_ptrs;
			m_ptrs = tmp;

			if (m_create_linear_idx == TRUE)
			{
				INDEX_TYPE *idx = new INDEX_TYPE[m_alloc_n+incr];
				ASSERT (idx);
				
				memmove (idx,m_idx,sizeof(INDEX_TYPE)*m_ptrs_n);
				memset (&idx[m_ptrs_n],0,sizeof(INDEX_TYPE)*incr);

				delete [] m_idx;
				m_idx = idx;

				// expand the alternate indexes
				for (INDEX_TYPE i_alt_index=0; i_alt_index<m_num_alt_indexes; i_alt_index++)
				{
					INDEX_TYPE *a_idx = m_alt_indexes[i_alt_index];
					if (m_ptrs_n)
						ASSERT(a_idx);

					INDEX_TYPE *idx = new INDEX_TYPE[m_alloc_n+incr];
					ASSERT (idx);
	
					if (m_ptrs_n)
						memmove (idx,a_idx,sizeof(INDEX_TYPE)*m_ptrs_n);

					memset (&idx[m_ptrs_n],0,sizeof(INDEX_TYPE)*incr);

					delete [] a_idx;
					m_alt_indexes[i_alt_index] = idx;
				}
			}

			m_alloc_n += incr;
		}

		return TRUE;
	}

	inline BOOL AppendIfUnique (PTR_CLASS *tr)
	{
		if (Exists (tr) == TRUE)
			return FALSE;

		return Append (tr);
	}

	inline BOOL Append (PTR_CLASS *tr)
	{
		ASSERT (m_block == 0);

		if (increase_allocation_if_necessary (m_ptrs_n+1) == FALSE)
			return FALSE;

		m_ptrs[m_ptrs_n] = tr;

		BOOL iret = UpdateIndex (m_ptrs_n,tr);
        if (iret == FALSE)
        {
            m_ptrs[m_ptrs_n] = 0;
            return FALSE;
        }

		m_ptrs_n++;

		return TRUE;
	}

    inline BOOL CycleIn (PTR_CLASS *tr, BOOL overwrite_existing=TRUE)
    {
		ASSERT (m_block == 0);

        if (m_create_linear_idx != TRUE)
        {
            ASSERT (FALSE); // Only supported for this index type
            return FALSE;
        }

        if (!(*tr > *m_ptrs[m_idx[m_ptrs_n-1]]))
        {
            ASSERT (FALSE); // The new item must be greater than any previous item
            return FALSE;
        }
        
        INDEX_TYPE ptr_to_replace_i = m_idx[0];
        
        // Assign the object
        if (overwrite_existing == TRUE)
        {
            *m_ptrs[ptr_to_replace_i] = *tr;
        }
        else
        if (m_delete_objects)
        {
            delete m_ptrs[ptr_to_replace_i];
            m_ptrs[ptr_to_replace_i] = tr;
        }

        // Update the index
        for (INDEX_TYPE i=0; i<m_ptrs_n-1; i++)
            m_idx[i] = m_idx[i+1];

        m_idx[m_ptrs_n-1] = ptr_to_replace_i;

        return TRUE;
    }

	inline BOOL Insert (INDEX_TYPE i, PTR_CLASS *tr)
	{
		ASSERT (m_block == 0);
		ASSERT (i <= m_ptrs_n);

		if (i > m_ptrs_n) 
			return FALSE;

		if (increase_allocation_if_necessary (m_ptrs_n+1) == FALSE)
			return FALSE;

		INDEX_TYPE j;
		for (j=m_ptrs_n; j>i; j--)
			m_ptrs[j] = m_ptrs[j-1];

		m_ptrs[i] = tr;

		BOOL iret = UpdateIndex (i,tr);
        if (iret == FALSE)
        {
            ///////////////////////////////////////
            // Undo
            for (j=i; j<m_ptrs_n; j++)
                m_ptrs[j] = m_ptrs[j+i];

            m_ptrs[m_ptrs_n] = 0;
            ////////////////////////////////////////

            return FALSE;
        }

		m_ptrs_n++;

		return TRUE;
	}

	inline PTR_CLASS* GetIfExists(PTR_CLASS* tr)
	{
		INDEX_TYPE idx;
		if (Exists(tr, &idx) == FALSE)
			return 0;

		return GetIndexedPtr(idx - 1);
	}

	inline const PTR_CLASS* GetIfExistsConst(const PTR_CLASS* tr, INDEX_TYPE *idx_ret=0) const
	{
		INDEX_TYPE idx;
		if (ExistsConst(tr, &idx) == FALSE)
			return 0;

		if (idx_ret)
			*idx_ret = idx - 1;

		return GetIndexedPtrConst(idx - 1);
	}

	inline BOOL Exists(PTR_CLASS *tr, INDEX_TYPE *idx = 0)
	{
		ASSERT(m_block == 0);
		if (m_create_linear_idx == TRUE)
		{
			BOOL exists;
			INDEX_TYPE i = GetIdxInsertPosition(tr, exists);

			if (idx)
				*idx = i;

			return exists;
		}

		for (INDEX_TYPE i = 0; i<m_ptrs_n; i++)
		{
			if ((*tr) < (*m_ptrs[i])) continue;
			if ((*tr) > (*m_ptrs[i])) continue;

			if (idx)
				*idx = i;

			return TRUE;
		}

		return FALSE;
	}

	inline BOOL ExistsConst(const PTR_CLASS *tr, INDEX_TYPE *idx = 0) const
	{
		ASSERT(m_block == 0);
		if (m_create_linear_idx == TRUE)
		{
			BOOL exists;
			INDEX_TYPE i = GetIdxInsertPositionConst(tr, exists);

			if (idx)
				*idx = i;

			return exists;
		}

		for (INDEX_TYPE i = 0; i<m_ptrs_n; i++)
		{
			if ((*tr) < (*m_ptrs[i])) continue;
			if ((*tr) > (*m_ptrs[i])) continue;

			if (idx)
				*idx = i;

			return TRUE;
		}

		return FALSE;
	}

	inline BOOL IsSorted (void)
	{
		ASSERT (m_block == 0);

		if (m_create_linear_idx)
		{
			ASSERT (FALSE);
			return FALSE;
		}

		for (INDEX_TYPE i=0; i<m_ptrs_n-1; i++)
		{
			if (*m_ptrs[i] > *m_ptrs[i+1])
				return FALSE;
		}

		return TRUE;
	}

	inline BOOL AddSorted (PTR_CLASS *obj)
	{
		ASSERT (m_block == 0);

		if (m_create_linear_idx)
		{
			ASSERT(FALSE);
			return FALSE;
		}

		if (increase_allocation_if_necessary (m_ptrs_n+1) == FALSE)
			return FALSE;

		for (INDEX_TYPE i=m_ptrs_n-1; i>=0; i--)
		{
			if (*obj < *m_ptrs[i])
			{
				m_ptrs[i+1] = m_ptrs[i];
				continue;
			}

			m_ptrs[i+1] = obj;
			m_ptrs_n++;
			return TRUE;
		}

		m_ptrs[0] = obj;
		m_ptrs_n++;
		return TRUE;
	}

	inline BOOL Sort (void)
	{
		ASSERT (m_block == 0);

		PTR_CLASS **tmp_ptrs = new PTR_CLASS*[m_ptrs_n];
		if (!tmp_ptrs)
			return FALSE;

		memset (tmp_ptrs,0,sizeof(PTR_CLASS*)*m_ptrs_n);

		if (m_create_linear_idx)
		{
			INDEX_TYPE *tmp_idx = new INDEX_TYPE[m_ptrs_n];
			if (!tmp_idx)
			{
				delete [] tmp_ptrs;
				return FALSE;
			}

			memset (tmp_idx,0,sizeof(INDEX_TYPE)*m_ptrs_n);

			for (INDEX_TYPE i=0; i<m_ptrs_n; i++)
			{
				tmp_idx[i] = i;

				PTR_CLASS *obj = GetIndexedPtr (i);
				tmp_ptrs[i] = obj;
			}

			memmove (m_idx,tmp_idx,sizeof(INDEX_TYPE)*m_ptrs_n);
			memmove (m_ptrs,tmp_ptrs,sizeof(PTR_CLASS*)*m_ptrs_n);

			delete [] tmp_idx;
			delete [] tmp_ptrs;

			return TRUE;
		}
		else
		{
			for (INDEX_TYPE i=0; i<m_ptrs_n; i++)
			{
				INDEX_TYPE j;
				for (j=0; j<i; j++)
				{
					if (*m_ptrs[i] < *m_ptrs[j])
					{
						for (INDEX_TYPE k=i; k>j; k--)
						{
							tmp_ptrs[k] = tmp_ptrs[k-1];
						}
						tmp_ptrs[j] = m_ptrs[i];
						break;
					}
				}

				if (j==i)
				{
					tmp_ptrs[j] = m_ptrs[j];
				}
			}

			memmove (m_ptrs,tmp_ptrs,sizeof(PTR_CLASS*)*m_ptrs_n);

			delete [] tmp_ptrs;

			return TRUE;
		}
	}

	inline BOOL UpdateIndex (const INDEX_TYPE &pos, PTR_CLASS *tr)
	{
		ASSERT (m_block == 0);

		if (m_create_linear_idx == TRUE)
		{
			BOOL exists;
			INDEX_TYPE i = GetIdxInsertPosition (tr,exists);

            if (exists)
            {
                if (m_unique)
                    return FALSE;
            }

			// Shift index values if necessary
			if (i < m_ptrs_n)
			{
				INDEX_TYPE j = m_ptrs_n;
				while (1)
				{
					if (j <= i) break;
					m_idx[j] = m_idx[j-1];
					j--;
				}
			}

			m_idx[i] = pos;

			// Update the alternate indexes
			for (INDEX_TYPE i_alt_index=0; i_alt_index<m_num_alt_indexes; i_alt_index++)
			{
				INDEX_TYPE *a_idx = m_alt_indexes[i_alt_index];
				ASSERT(a_idx);

				i = GetAltIdxInsertPosition (i_alt_index,tr,exists);

				// Shift index values if necessary
				if (i < m_ptrs_n)
				{
					INDEX_TYPE j = m_ptrs_n;
					while (1)
					{
						if (j <= i) break;
						a_idx[j] = a_idx[j-1];
						j--;
					}
				}

				a_idx[i] = pos;
			}

			return TRUE;
		}

        return TRUE;
	}

	inline PTR_CLASS *GetPtrByToken (PTR_CLASS *token, BOOL &exists)
	{
		ASSERT (m_block == 0);

		INDEX_TYPE idx = GetIdxInsertPosition (token,exists);
		if (idx == 0) return 0;

		PTR_CLASS *item = GetIndexedPtr (idx-1);
		if (*token < *item)
			return 0;

		if (*token > *item)
			return 0;

		return item;
	}

	inline INDEX_TYPE GetAltIdxInsertPosition (INDEX_TYPE i_alt_index, PTR_CLASS *tr, BOOL &exists)
	{
		exists = FALSE;

		if (i_alt_index < 0 || i_alt_index >= m_num_alt_indexes)
		{
			ASSERT(FALSE);
			return 0;
		}

		ASSERT(m_create_linear_idx);

		int (*Compare)(void *obj0,void *obj1) = m_alt_indexes_compare_funcs[i_alt_index].Compare;
		ASSERT(Compare);

		INDEX_TYPE *a_idx = m_alt_indexes[i_alt_index];
		ASSERT(a_idx);

		if (!m_ptrs_n) 
			return 0;

		// Check to see if this is greater than the last item
		int icmp = Compare (tr,m_ptrs[a_idx[m_ptrs_n-1]]);

		if (icmp == 0)
		{
			exists = TRUE;
			return m_ptrs_n;
		}

		if (icmp > 0)
			return m_ptrs_n;

		INDEX_TYPE i;

		if (m_ptrs_n < 6)
		{
			for (i = 0; i < m_ptrs_n-1; i++)
			{
				icmp = Compare (tr,m_ptrs[a_idx[i]]);

				if (icmp < 0)
					return i;

				if (exists == FALSE && icmp == 0)
					exists = TRUE;
			}

			return i;
		}


		INDEX_TYPE istart,iend;

		istart = 0;
		iend = m_ptrs_n-1;
		i = m_ptrs_n / 2;

		while (1)
		{
			icmp = Compare (tr,m_ptrs[a_idx[i]]);
			if (icmp < 0)
				iend = i - 1;
			else
				istart = i + 1;

			if (iend - istart < 4)
			{
				for (i = istart; i <= iend; i++)
				{
					icmp = Compare (tr,m_ptrs[a_idx[i]]);

					if (icmp < 0)
					{
						if (exists == FALSE && i && !(Compare (tr,m_ptrs[a_idx[i-1]]) > 0))
							exists = TRUE;

						return i;
					}

					if (exists == FALSE && icmp == 0)
						exists = TRUE;
				}

				return i;
			}

			i = (iend - istart) / 2;
			i += istart;
		}
	}

	inline BOOL GetStartAndEndIndexes (PTR_CLASS &token, INDEX_TYPE &istart, INDEX_TYPE &iend)
	{
		BOOL exists;
		INDEX_TYPE i = GetIdxInsertPosition (&token, exists);
		if (exists == FALSE)
			return FALSE;

		i--;

		iend = i;
		istart = i;

		while (i >= 0 && i < GetCount ())
		{
			PTR_CLASS *obj = GetIndexedPtr (i);
			if (*obj < token)
				break;

			istart = i;
			i--;
		}

		return TRUE;
	}

	inline BOOL GetAltStartAndEndIndexes (INDEX_TYPE i_alt_index, PTR_CLASS &token, INDEX_TYPE &istart, INDEX_TYPE &iend)
	{
		BOOL exists;
		INDEX_TYPE i = GetAltIdxInsertPosition (i_alt_index, &token, exists);
		if (exists == FALSE)
			return FALSE;

		i--;

		iend = i;
		istart = i;

		while (i >= 0 && i < GetCount ())
		{
			PTR_CLASS *obj = GetAltIndexedPtr (i_alt_index,i);

			if (m_alt_indexes_compare_funcs[i_alt_index].Compare (obj,&token) < 0)
				break;

			istart = i;
			i--;
		}

		return TRUE;
	}

	inline INDEX_TYPE GetIdxInsertPosition(PTR_CLASS *tr, BOOL &exists, INDEX_TYPE *raw_idx = 0)
	{
		ASSERT(m_block == 0);

		exists = FALSE;

		if (m_create_linear_idx == TRUE)
		{
			if (!m_ptrs_n)
				return 0;

			// Check to see if this is greater than the last item
			if (!(*tr < *m_ptrs[m_idx[m_ptrs_n - 1]]))
			{
				if (!(*tr > *m_ptrs[m_idx[m_ptrs_n - 1]]))
				{
					exists = TRUE;
					if (raw_idx)
						*raw_idx = m_idx[m_ptrs_n - 1];
				}

				return m_ptrs_n;
			}

			INDEX_TYPE i;

			if (m_ptrs_n < 6)
			{
				for (i = 0; i < m_ptrs_n - 1; i++)
				{
					if (*tr < *m_ptrs[m_idx[i]])
						return i;

					if (exists == FALSE && !(*tr > *m_ptrs[m_idx[i]]))
					{
						if (raw_idx)
							*raw_idx = m_idx[i];
						exists = TRUE;
					}
				}

				return i;
			}


			INDEX_TYPE istart, iend;

			istart = 0;
			iend = m_ptrs_n - 1;
			i = m_ptrs_n / 2;

			while (1)
			{
				if (*tr < *m_ptrs[m_idx[i]])
					iend = i - 1;
				else
					istart = i + 1;

				if (iend - istart < 4)
				{
					for (i = istart; i <= iend; i++)
					{
						if (*tr < *m_ptrs[m_idx[i]])
						{
							if (exists == FALSE && i && !(*tr > *m_ptrs[m_idx[i - 1]]))
							{
								if (raw_idx)
									*raw_idx = m_idx[i - 1];

								exists = TRUE;
							}

							return i;
						}

						if (exists == FALSE && !(*tr > *m_ptrs[m_idx[i]]))
						{
							if (raw_idx)
								*raw_idx = m_idx[i];

							exists = TRUE;
						}
					}

					return i;
				}

				i = (iend - istart) / 2;
				i += istart;
			}
		}

		ASSERT(FALSE);

		return 0;
	}

	inline INDEX_TYPE GetIdxInsertPositionConst(const PTR_CLASS *tr, BOOL &exists, INDEX_TYPE *raw_idx = 0) const
	{
		ASSERT(m_block == 0);

		exists = FALSE;

		if (m_create_linear_idx == TRUE)
		{
			if (!m_ptrs_n)
				return 0;

			// Check to see if this is greater than the last item
			if (!(*tr < *m_ptrs[m_idx[m_ptrs_n - 1]]))
			{
				if (!(*tr > *m_ptrs[m_idx[m_ptrs_n - 1]]))
				{
					exists = TRUE;
					if (raw_idx)
						*raw_idx = m_idx[m_ptrs_n - 1];
				}

				return m_ptrs_n;
			}

			INDEX_TYPE i;

			if (m_ptrs_n < 6)
			{
				for (i = 0; i < m_ptrs_n - 1; i++)
				{
					if (*tr < *m_ptrs[m_idx[i]])
						return i;

					if (exists == FALSE && !(*tr > *m_ptrs[m_idx[i]]))
					{
						if (raw_idx)
							*raw_idx = m_idx[i];
						exists = TRUE;
					}
				}

				return i;
			}


			INDEX_TYPE istart, iend;

			istart = 0;
			iend = m_ptrs_n - 1;
			i = m_ptrs_n / 2;

			while (1)
			{
				if (*tr < *m_ptrs[m_idx[i]])
					iend = i - 1;
				else
					istart = i + 1;

				if (iend - istart < 4)
				{
					for (i = istart; i <= iend; i++)
					{
						if (*tr < *m_ptrs[m_idx[i]])
						{
							if (exists == FALSE && i && !(*tr > *m_ptrs[m_idx[i - 1]]))
							{
								if (raw_idx)
									*raw_idx = m_idx[i - 1];

								exists = TRUE;
							}

							return i;
						}

						if (exists == FALSE && !(*tr > *m_ptrs[m_idx[i]]))
						{
							if (raw_idx)
								*raw_idx = m_idx[i];

							exists = TRUE;
						}
					}

					return i;
				}

				i = (iend - istart) / 2;
				i += istart;
			}
		}

		ASSERT(FALSE);

		return 0;
	}

	inline BOOL Transfer (PtrSet<PTR_CLASS,INDEX_TYPE> &list)
	{
		ASSERT (m_block == 0);

		BOOL list_status_delete_objects = list.GetStatusDeleteObjects ();
		list.SetStatusDeleteObjects (FALSE);
		
		for (INDEX_TYPE i=0; i < list.GetCount (); i++)
		{
			PTR_CLASS *ptr = list.GetObject (i);
			ASSERT (ptr);
			if (!ptr) continue;

			Append (ptr);
		}

		list.RemoveAll ();
		list.SetStatusDeleteObjects (list_status_delete_objects);

		return TRUE;
	}

	inline BOOL Copy (const PtrSet<PTR_CLASS,INDEX_TYPE> &list)
	{
		for (INDEX_TYPE i=0; i < list.GetCount (); i++)
		{
			const PTR_CLASS *ptr = list.GetObjectConst (i);
			PTR_CLASS *new_ptr = new PTR_CLASS;
			new_ptr->Assign (*ptr);
			Append (new_ptr);
		}

		return TRUE;
	}

	inline BOOL IsIdentical (PtrSet<PTR_CLASS,INDEX_TYPE> &list)
	{
		if (list.GetCount () != GetCount ())
			return FALSE;

		for (INDEX_TYPE i=0; i<GetCount (); i++)
		{
			PTR_CLASS *obj0 = GetObject (i);
			PTR_CLASS *obj1 = list.GetObject (i);

			if (*obj0 < *obj1)
				return FALSE;

			if (*obj0 > *obj1)
				return FALSE;
		}

		return TRUE;
	}

    inline BOOL Remove (PTR_CLASS *ptr)
    {
		ASSERT (m_block == 0);

        BOOL exists;
        INDEX_TYPE idx;

        exists = Exists (ptr,&idx);
        if (!exists)
            return FALSE;

        BOOL iret = RemoveByIdx (idx-1);
        return iret;
    }

    inline BOOL RemoveByIdx (const INDEX_TYPE &idx)
    {
		ASSERT (m_block == 0);

		if (m_create_linear_idx == TRUE)
        {
            INDEX_TYPE i = m_idx[idx];
            BOOL iret = Remove (i);
            return iret;
        }

        BOOL iret = Remove (idx);
        return iret;
    }

	inline BOOL Remove (INDEX_TYPE i)
	{
		if (i < 0 || i >= m_ptrs_n)
			return FALSE;

		ASSERT (m_block == 0);

		ASSERT (i < m_ptrs_n);

		if (i >= m_ptrs_n) 
			return FALSE;

		INDEX_TYPE j;

		if (m_delete_objects == TRUE)
		{
			delete m_ptrs[i]; 
			m_ptrs[i] = 0;
		}

		for (j=i; j<m_ptrs_n-1; j++)
			m_ptrs[j] = m_ptrs[j+1];

		if (m_create_linear_idx == TRUE)
		{
			BOOL shift=FALSE;

			for (j=0; j<m_ptrs_n-1; j++)
			{
				if (m_idx[j] == i)
					shift = TRUE;

				// shift index values
				if (shift == TRUE)
					m_idx[j] = m_idx[j+1];

				// Adjust for new m_ptrs_n count
				if (m_idx[j] > i)
					m_idx[j]--;
			}

			for (INDEX_TYPE i_alt_indexes=0; i_alt_indexes<m_num_alt_indexes; i_alt_indexes++)
			{
				shift=FALSE;

				for (j=0; j<m_ptrs_n-1; j++)
				{
					if (m_alt_indexes[i_alt_indexes][j] == i)
						shift = TRUE;

					// shift index values
					if (shift == TRUE)
						m_alt_indexes[i_alt_indexes][j] = m_alt_indexes[i_alt_indexes][j+1];

					// Adjust for new m_ptrs_n count
					if (m_alt_indexes[i_alt_indexes][j] > i)
						m_alt_indexes[i_alt_indexes][j]--;
				}
			}
		}

		m_ptrs_n--;
		m_ptrs[m_ptrs_n] = 0;

		return TRUE;
	}

	inline INDEX_TYPE GetIndex (INDEX_TYPE i)
	{
		ASSERT (m_block == 0);

		ASSERT (i >= 0 && i < m_ptrs_n);
		return m_idx[i];
	}

	inline INDEX_TYPE GetRawIndex (INDEX_TYPE idx)
	{
		ASSERT (m_block == 0);
		ASSERT (idx >= 0 && idx < m_ptrs_n);
		
		if (m_create_linear_idx == FALSE)
			return idx;

		PTR_CLASS *obj = GetIndexedPtr (idx); 
		BOOL exists;
		INDEX_TYPE raw_idx;
		INDEX_TYPE idx1 = GetIdxInsertPosition (obj, exists, &raw_idx);

		ASSERT (exists);
		
		return raw_idx;
	}

	inline INDEX_TYPE GetRawIndexDirectFromAlt (INDEX_TYPE i_alt_index, INDEX_TYPE idx)
	{
		ASSERT (m_block == 0);

		ASSERT (idx >= 0 && idx < m_ptrs_n);

		if (i_alt_index < 0 || i_alt_index >= m_num_alt_indexes)
			ASSERT(FALSE);

		return m_alt_indexes[i_alt_index][idx];
	}

	inline INDEX_TYPE GetRawIndexDirect (INDEX_TYPE idx)
	{
		ASSERT (m_block == 0);

		if (m_create_linear_idx == FALSE)
			return idx;

		ASSERT (idx >= 0 && idx < m_ptrs_n);

		return m_idx[idx];
	}
		
	inline PTR_CLASS *GetIndexedPtr (INDEX_TYPE i)
	{
		ASSERT (m_block == 0);

		if (i < 0)
			return 0;

		if (i >= m_ptrs_n)
			return 0;

		if (m_create_linear_idx == TRUE)
			return m_ptrs[m_idx[i]];
		
		ASSERT (FALSE);
		return m_ptrs[i];
	}

	inline const PTR_CLASS *GetIndexedPtrConst (INDEX_TYPE i) const
	{
		ASSERT (m_block == 0);		
		
		if (i < 0)
			return 0;

		if (i >= m_ptrs_n)
			return 0;
		
		if (m_create_linear_idx == TRUE)
			return m_ptrs[m_idx[i]];
		
		ASSERT (FALSE);
		return m_ptrs[i];
	}

	inline PTR_CLASS *GetAltIndexedPtr (INDEX_TYPE i_alt_index, INDEX_TYPE i)
	{
		ASSERT (m_block == 0);

		ASSERT (i >= 0 && i < m_ptrs_n);

		if (i_alt_index < 0 || i_alt_index >= m_num_alt_indexes)
		{
			ASSERT(FALSE);
			return 0;
		}

		if (m_create_linear_idx == TRUE)
			return m_ptrs[m_alt_indexes[i_alt_index][i]];
		
		ASSERT (FALSE);
		return m_ptrs[i];
	}

	inline INDEX_TYPE GetAltRawIndexDirect (INDEX_TYPE i_alt_index, INDEX_TYPE i)
	{

		ASSERT (m_block == 0);

		ASSERT (i >= 0 && i < m_ptrs_n);

		if (i_alt_index < 0 || i_alt_index >= m_num_alt_indexes)
		{
			ASSERT(FALSE);
			return 0;
		}

		if (m_create_linear_idx == TRUE)
			return m_alt_indexes[i_alt_index][i];
		
		ASSERT (FALSE);
		return i;
	}

	inline BOOL RemoveAll (void)
	{
		if (m_block)
		{
			delete [] m_block;
			m_block = 0;
			m_block_sz = 0;
			return TRUE;
		}

		//ASSERT (m_block == 0);

		delete_pointers ();
		return TRUE;
	}

	inline const INDEX_TYPE &GetAllocCount (void)
	{
		ASSERT (m_block == 0);

		return m_alloc_n;
	}

	inline PTR_CLASS *GetPtr (INDEX_TYPE i)
	{
		if (m_block)
		{
			if (i >= m_block_sz)
				return 0;

			return &m_block[i];
		}

		if (i >= m_ptrs_n) return 0;
		if (!m_ptrs) return 0;
		return m_ptrs[i];
	}

	inline void ExtractAll (void)
	{
		ASSERT (m_block == 0);

		while (m_ptrs_n)
			ExtractLastPtr ();
	}

	inline PTR_CLASS *ExtractLastPtr (void)
	{
		ASSERT (m_block == 0);

		return ExtractObject (m_ptrs_n-1);
	}

	inline INDEX_TYPE GetRangeMatchingToken (PTR_CLASS &token, INDEX_TYPE &istart, INDEX_TYPE &iend)
	{
		BOOL exists = FALSE;
		iend = GetIdxInsertPosition (&token,exists);
		iend--;

		if (exists == FALSE)
		{
			istart = iend;
			return 0;
		}

		istart = iend;
		while (istart > 0)
		{
			PTR_CLASS *obj  = GetIndexedPtr (istart-1);
			if (*obj < token)
				break;
			
			if (istart == 0)
				break;

			istart--;
		}

		INDEX_TYPE n = iend - istart + 1;

		return n;
	}

	inline PTR_CLASS *ExtractObject (INDEX_TYPE i)
	{
		ASSERT (m_block == 0);

		if (!m_ptrs) return 0;
		if (i >= m_ptrs_n) return 0;

		BOOL delete_objects_prev = m_delete_objects;
		m_delete_objects = FALSE;

		PTR_CLASS *obj = m_ptrs[i];

		if (Remove (i) != TRUE)
		{
			m_delete_objects = delete_objects_prev;
			return 0;
		}

		m_delete_objects = delete_objects_prev;

		return obj;
	}

	// obj must be equal to existing m_ptrs[i] for indexing purposes
	inline PTR_CLASS *SwapInObject (INDEX_TYPE i, PTR_CLASS *obj)
	{
		ASSERT (m_block == 0);

		if (i >= m_ptrs_n) return 0;

		while (1)
		{
			if (m_create_linear_idx == TRUE)
			{
				if (*obj > *(m_ptrs[i])) break;
				if (*obj < *(m_ptrs[i])) break;
			}

			// No need to re-index
			PTR_CLASS *old_obj = m_ptrs[i];
			m_ptrs[i] = obj;

			return old_obj;
		}

		// Must re-index
		if (Append (obj) == FALSE)
			return 0;

		return ExtractObject (i);
	}

	inline const int32_t LoadFromBufferBlockMode (const void *v)
	{
		delete_pointers ();

		m_memory_block_mode = TRUE;
		SetStatusCreateLinearIndex (FALSE);

		const BYTE *b = (const BYTE *) v;

		int32_t i=0;
		
		// Extract the number of objects from the buffer
		INDEX_TYPE n;
 		memmove (&n,b+i,sizeof (n));
		i += sizeof (n);

		PTR_CLASS *block = new PTR_CLASS[n];
		ASSERT(block);

		for (INDEX_TYPE j=0; j<n; j++)
		{
			// Initialize each object from the buffer
			i += block[j].LoadFromBuffer ((const char *) (b+i));
		}

		m_block_sz = n;
		m_block = block;

		return i;
	}

	inline const int32_t LoadFromBuffer (const void *v)
	{
		ASSERT (m_block == 0);

		delete_pointers ();

		const BYTE *b = (const BYTE *) v;

		int32_t i=0;
		
		// Extract the number of objects from the buffer
		INDEX_TYPE ptrs_n;
 		memmove (&ptrs_n,b+i,sizeof (ptrs_n)); 
		i += sizeof (ptrs_n);
		
		increase_allocation_if_necessary (ptrs_n);

        PTR_CLASS *ptr;

		for (INDEX_TYPE j=0; j<ptrs_n; j++)
		{
			// Allocate each object
            ptr = new PTR_CLASS;
            ASSERT (ptr);

			// Initialize each object from the buffer
			i += ptr->LoadFromBuffer ((const BYTE *) (b+i));

            Append (ptr);
		}

		return i;
	}
		
	inline const int32_t SaveToBuffer (void *v) const 
	{
		ASSERT (m_block == 0);

		BYTE *b = (BYTE *) v;

		int32_t i=0;

		memmove (b+i,&m_ptrs_n,sizeof (m_ptrs_n));	
		i += sizeof (m_ptrs_n);

	    for (INDEX_TYPE j=0; j<(int32_t)m_ptrs_n; j++)
		{
			i += m_ptrs[j]->SaveToBuffer (b+i);
		}

		return i;
	}

	inline const int32_t GetStorageSizeBytes (void) const
	{
		ASSERT (m_block == 0);

		int32_t i=0;

		i += sizeof (m_ptrs_n);

		for (INDEX_TYPE j=0; j<m_ptrs_n; j++)
			i += m_ptrs[j]->GetStorageSizeBytes ();

		return i;
	}

	inline BOOL SetCount (const INDEX_TYPE count)
	{
		ASSERT (m_block == 0);

		delete_pointers ();
		increase_allocation_if_necessary (count);
		for (INDEX_TYPE i=0; i<count; i++)
		{
			m_ptrs[i] = new PTR_CLASS;
			ASSERT (m_ptrs[i]);
		}
		m_ptrs_n = count;
		return TRUE;
	}

	inline INDEX_TYPE GetCount (void) const
	{
		if (m_block)
			return m_block_sz;

		return m_ptrs_n;
	}

	inline PTR_CLASS *GetObject (const INDEX_TYPE j)
	{
		if (m_block)
		{
			if (j<0 || j>=m_block_sz)
				return 0;

			return &m_block[j];
		}

		if (j<0 || j>m_ptrs_n-1) return 0;
		return m_ptrs[j];
	}

	inline const PTR_CLASS *GetObjectConst (const INDEX_TYPE j) const
	{
		if (m_block)
		{
			if (j<0 || j>=m_block_sz)
				return 0;

			return &m_block[j];
		}

		if (j<0 || j>m_ptrs_n-1) return 0;
		return m_ptrs[j];
	}

	inline BOOL Assign (const INDEX_TYPE j, const void *buffer)
	{
		ASSERT (m_block == 0);

		if (j<0 || j>m_ptrs_n-1) 
            return FALSE;
		return TRUE;
	}

    inline BOOL Assign (const PtrSet<PTR_CLASS,INDEX_TYPE> &tr)
    {
		ASSERT (m_block == 0);

        int32_t n = tr.GetStorageSizeBytes ();
        BYTE *b = new BYTE[n];
        if (!b) return FALSE;
        int32_t check = tr.SaveToBuffer (b);
        ASSERT (check == n);
        check = LoadFromBuffer (b);
        ASSERT (check == n);
        delete b;
        return TRUE;
    }

	inline BOOL LoadFromFile (const char *file_name, string &err_msg)
	{
		HANDLE h = CreateFile (file_name,GENERIC_READ,NULL,NULL,OPEN_EXISTING,NULL,NULL);

		if (h == INVALID_HANDLE_VALUE)
		{
			err_msg = "Unable to open file: ";
			err_msg += file_name;

			DWORD err = GetLastError ();

			return FALSE;
		}

		int32_t sz = (int32_t)GetFileSize(h,NULL);

		BYTE *buffer = new BYTE[sz];
		if (!buffer)
		{
			CloseHandle (h);
			err_msg = "Unable to allocate enough memory.";
			return FALSE;
		}

		DWORD nread = 0;
		if (ReadFile (h,buffer,sz,&nread,NULL) == FALSE)
		{
			err_msg = "Problem reading file: ";
			err_msg += file_name;
			CloseHandle (h);
			delete [] buffer;
			return FALSE;
		}

		int32_t check = LoadFromBuffer (buffer);
		ASSERT (check == (int32_t)sz);

		CloseHandle (h);
		delete [] buffer;
		return TRUE;
	}

	inline BOOL SaveToFile (const char *file_name, string &err_msg)
	{
		HANDLE h = CreateFile (file_name,GENERIC_WRITE,NULL,NULL,CREATE_ALWAYS,NULL,NULL);

		if (h == INVALID_HANDLE_VALUE)
		{
			err_msg = "Unable to create file: ";
			err_msg += file_name;

			DWORD err = GetLastError ();

			return FALSE;
		}

		int32_t sz = GetStorageSizeBytes ();
		BYTE *buffer = new BYTE[sz];
		if (!buffer)
		{
			CloseHandle (h);
			err_msg = "Unable to allocate enough memory.";
			return FALSE;
		}

		int32_t check = SaveToBuffer (buffer);
		ASSERT (check == sz);

		DWORD nwritten = 0;
		if (WriteFile (h,buffer,sz,&nwritten,NULL) == FALSE)
		{
			err_msg = "Problem writing file: ";
			err_msg += file_name;
			CloseHandle (h);
			delete [] buffer;
			return FALSE;
		}

		CloseHandle (h);
		delete [] buffer;
		return TRUE;
	}

	inline BOOL SetMinSize (INDEX_TYPE sz)
	{
		if (GetCount () >= sz)
			return TRUE;

		while (GetCount () < sz)
		{
			PTR_CLASS *obj = new PTR_CLASS;
			if (Append (obj) == FALSE)
				return FALSE;
		}

		return TRUE;
	}

	inline BOOL ReIndex (INDEX_TYPE *raw_index=0)
	{
		if (m_create_linear_idx == FALSE)
			return TRUE;

		if (raw_index == 0)
		{
			PtrSet<PTR_CLASS,INDEX_TYPE> tmp;

			tmp.SetStatusCreateLinearIndex (FALSE);
			if (tmp.Transfer (*this) == FALSE)
				return FALSE;

			return Transfer (tmp);
		}

		PTR_CLASS *obj = ExtractObject (*raw_index);
		if (!obj)
			return FALSE;

		return Append (obj);
	}

	inline BOOL ReIndexIfNecessary (void)
	{
		INDEX_TYPE N = GetCount ();
		
		PTR_CLASS *_obj = GetIndexedPtr (0);
		for (INDEX_TYPE i=1; i<N; i++)
		{
			PTR_CLASS *obj = GetIndexedPtr (i);
			if (obj < _obj)
				return ReIndex ();

			_obj = obj;
		}

		return TRUE;
	}

private:

	inline void delete_pointers (BOOL clear_alternative_indexes=FALSE)
	{
		if (m_delete_objects == TRUE)
		{
			for (INDEX_TYPE i=0; i<m_ptrs_n; i++)
				delete m_ptrs[i];
		}
		else
		{
			BYTE break_here = TRUE;
		}

		if (m_ptrs)
		{
			delete [] m_ptrs;
			m_ptrs = 0;
		}
		
		if (m_idx)
		{
			delete [] m_idx;
			m_idx = 0;
		}
		
		if (m_block)
		{
			delete [] m_block;
			m_block = 0;
		}

		m_ptrs_n = 0;
		m_alloc_n = 0;

		if (m_alt_indexes)
		{
			for (INDEX_TYPE i=0; i<m_num_alt_indexes; i++)
			{
				delete [] m_alt_indexes[i];
				m_alt_indexes[i] = 0;
			}

			if (clear_alternative_indexes)
			{
				delete [] m_alt_indexes;
				m_alt_indexes = 0;
			}
		}

		if (m_alt_indexes_compare_funcs && clear_alternative_indexes)
		{
			delete [] m_alt_indexes_compare_funcs;
			m_alt_indexes_compare_funcs = 0;
		}

		if (clear_alternative_indexes)
			m_num_alt_indexes = 0;
	}

};

template <class ARRAY_TYPE, class CAST_AS_TYPE, class DATA_TYPE> class BinSearch
{
public:
	inline int32_t GetIndex (ARRAY_TYPE *array, const int32_t array_size_n, 
				   const DATA_TYPE &target)
	{
		uint32_t i;
		for (i=0; i<array_size_n; i++)
		{
			if ((DATA_TYPE)((CAST_AS_TYPE &)array[i]) >= target)
			{
				if ((DATA_TYPE)((CAST_AS_TYPE &)array[i]) == target)
					return i;

				return (i-1);
			}
		}

		return i;
	}
};

inline DWORD GetCheckSumB (const void *buffer, DWORD sz_bytes)
{
	DWORD checksum=0;

	const DWORD *u4 = (const DWORD *) buffer;

	DWORD n = sz_bytes/4;
	DWORD r = sz_bytes%4;

	DWORD i;
	for (i=0; i<n; i++)
		checksum += u4[i];

	if (r)
	{
		const BYTE *u1 = (const BYTE *) &u4[n];
		for (i=0; i<r; i++)
		{
			checksum += u1[i];
		}
	}

	return checksum;
}

template <class OBJ> class CheckSum
{
public:

	inline CheckSum (void)
	{
		m_has_been_computed = FALSE;
		m_checksum = 0;
	}

	inline void Compute (OBJ &obj)
	{
		int32_t sz = obj.GetStorageSizeBytes ();
		BYTE *buffer = new BYTE[sz];
		ASSERT (buffer);
		int32_t chk = obj.SaveToBuffer (buffer);
		ASSERT (chk == sz);

		m_checksum = GetCheckSumB (buffer,sz);
		m_has_been_computed = TRUE;

		delete [] buffer;
	}

	inline BOOL HasBeenComputed (void)
	{
		return m_has_been_computed;
	}

	inline BOOL IsEqualTo (CheckSum<OBJ> &cs_obj)
	{
		if (!m_has_been_computed)
			return FALSE;

		if (cs_obj.HasBeenComputed () == FALSE)
			return FALSE;

		if (m_checksum != cs_obj.GetCheckSum ())
			return FALSE;

		return TRUE;
	}

	inline DWORD GetCheckSum (void) 
	{
		return m_checksum;
	}

protected:

	BOOL m_has_been_computed;
	DWORD m_checksum;

};


}; //namespace WorkbenchLib

