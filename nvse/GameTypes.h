#pragma once

#include <list>
#include "Utilities.h"
#include "NiTypes.h"

// 8
class String
{
public:
	String();
	~String();

	char	* m_data;
	UInt16	m_dataLen;
	UInt16	m_bufLen;

	bool	Set(const char * src);
	bool	Includes(const char* toFind) const;
	bool	Replace(const char* toReplace, const char* replaceWith); // replaces instance of toReplace with replaceWith
	bool	Append(const char* toAppend);
	double	Compare(const String& compareTo, bool caseSensitive = false);

	const char *	CStr(void);
};

enum {
	eListCount = -3,
	eListEnd = -2,
	eListInvalid = -1,		
};

typedef void * (* _FormHeap_Allocate)(UInt32 size);
extern const _FormHeap_Allocate FormHeap_Allocate;

typedef void (* _FormHeap_Free)(void * ptr);
extern const _FormHeap_Free FormHeap_Free;

typedef TESForm * (* _LookupFormByID)(UInt32 id);
extern const _LookupFormByID LookupFormByID;

template <typename T_Data> struct ListNode
{
	T_Data* data;
	ListNode* next;

	ListNode() : data(NULL), next(NULL) {}
	ListNode(T_Data* _data) : data(_data), next(NULL) {}

	T_Data* Data() const { return data; }
	ListNode* Next() const { return next; }

	ListNode* RemoveMe()
	{
		if (next)
		{
			ListNode* pNext = next;
			data = next->data;
			next = next->next;
			FormHeap_Free(pNext);
			return this;
		}
		data = NULL;
		return NULL;
	}

	ListNode* RemoveNext()
	{
		ListNode* pNext = next;
		next = next->next;
		FormHeap_Free(pNext);
		return next;
	}

	ListNode* Append(T_Data* _data)
	{
		ListNode* newNode = (ListNode*)FormHeap_Allocate(sizeof(ListNode));
		newNode->data = _data;
		newNode->next = next;
		next = newNode;
		return newNode;
	}

	ListNode* Insert(T_Data* _data)
	{
		ListNode* newNode = (ListNode*)FormHeap_Allocate(sizeof(ListNode));
		newNode->data = data;
		data = _data;
		newNode->next = next;
		next = newNode;
		return newNode;
	}
};

template <class Item> class tList
{
public:
	typedef ListNode<Item> Node;

private:
	Node	m_listHead;

	template <class Op>
	UInt32 FreeNodes(Node* node, Op& compareOp) const
	{
		static UInt32 nodeCount = 0, numFreed = 0, lastNumFreed = 0;
		if (node->next)
		{
			nodeCount++;
			FreeNodes(node->next, compareOp);
			nodeCount--;
		}
		if (compareOp.Accept(node->data))
		{
			node->RemoveMe();
			numFreed++;
		}
		if (!nodeCount)
		{
			lastNumFreed = numFreed;
			numFreed = 0;
		}
		return lastNumFreed;
	}

public:
	void Init(Item* item = NULL)
	{
		m_listHead.data = item;
		m_listHead.next = NULL;
	}

	Node* GetLastNode(SInt32* outIdx = NULL) const
	{
		SInt32 index = 0;
		Node* node = Head();
		while (node->next)
		{
			node = node->next;
			index++;
		}
		if (outIdx) *outIdx = index;
		return node;
	}

	Node* GetNthNode(SInt32 index) const
	{
		if (index >= 0)
		{
			Node* node = Head();
			do
			{
				if (!index) return node;
				index--;
			} while (node = node->next);
		}
		return NULL;
	}

	Node* Head() const { return const_cast<Node*>(&m_listHead); }

	bool IsEmpty() const { return !m_listHead.data && !m_listHead.next; }

	class Iterator
	{
		Node* m_curr;

	public:
		Iterator operator++()
		{
			if (m_curr) m_curr = m_curr->next;
			return *this;
		}
		bool End() const { return !m_curr || (!m_curr->data && !m_curr->next); }
		Item* operator->() const { return m_curr->data; }
		Item*& operator*() const { return m_curr->data; }
		const Iterator& operator=(const Iterator& rhs)
		{
			m_curr = rhs.m_curr;
			return *this;
		}
		bool operator==(const Iterator& other) const
		{
			return m_curr == other.m_curr;
		}

		bool operator!=(const Iterator& other) const
		{
			return m_curr != other.m_curr;
		}

		Item* Get() const { return m_curr->data; }
		void Next() { if (m_curr) m_curr = m_curr->next; }
		void Find(Item* _item)
		{
			while (m_curr)
			{
				if (m_curr->data == _item) break;
				m_curr = m_curr->next;
			}
		}

		Iterator(Node* node = NULL) : m_curr(node) {}
		Iterator(tList& _list) : m_curr(&_list.m_listHead) {}
		Iterator(tList* _list) : m_curr(&_list->m_listHead) {}
		Iterator(tList& _list, Item* _item) : m_curr(&_list.m_listHead) { Find(_item); }
		Iterator(tList* _list, Item* _item) : m_curr(&_list->m_listHead) { Find(_item); }
	};

	const Iterator Begin() const { return Iterator(Head()); }
	const Iterator begin() const { return Iterator(Head()); }
	const Iterator end() const { return Iterator(nullptr); }

	UInt32 Count() const
	{
		if (!m_listHead.data) return 0;
		Node* node = Head();
		UInt32 count = 1;
		while (node = node->next) count++;
		return count;
	};

	bool Contains(Item* item) const
	{
		Node* node = Head();
		do
		{
			if (node->data == item) return true;
			node = node->next;
		} while (node);
		return false;
	}

	Item* GetFirstItem() const
	{
		return m_listHead.data;
	}

	Item* GetLastItem() const
	{
		return GetLastNode()->data;
	}

	Item* GetNthItem(SInt32 index) const
	{
		if (eListEnd == index)
			return GetLastNode()->data;
		Node* node = GetNthNode(index);
		return node ? node->data : NULL;
	}

	Item* GetRandomItem() const
	{
		int numItems = Count();
		if (!numItems)
		{
			return nullptr;
		}

		int itemIdx = rand() % numItems;
		return GetNthItem(itemIdx);
	}

	SInt32 AddAt(Item* item, SInt32 index)
	{
		if (!item) return eListInvalid;
		Node* node;
		if (!index)
		{
			if (m_listHead.data) m_listHead.Insert(item);
			else m_listHead.data = item;
		}
		else if (eListEnd == index)
		{
			node = GetLastNode(&index);
			if (node->data) node->Append(item);
			else node->data = item;
		}
		else
		{
			node = GetNthNode(index);
			if (!node) return eListInvalid;
			node->Insert(item);
		}
		return index;
	}

	SInt32 Append(Item* item)
	{
		SInt32 index = eListInvalid;
		if (item)
		{
			Node* node = GetLastNode(&index);
			if (node->data) node->Append(item);
			else node->data = item;
		}
		return index;
	}

	void Insert(Item* item)
	{
		if (item)
		{
			if (m_listHead.data) m_listHead.Insert(item);
			else m_listHead.data = item;
		}
	}

	void InsertSorted(Item* item, bool (*Compare)(Item* a, Item* b))
	{
		Node* curr = Head();
		while (curr)
		{
			if (!curr->data || !Compare(item, curr->data)) break;
			curr = curr->next;
		}
		curr->Insert(item);
	}

	void CopyFrom(tList& sourceList)
	{
		Node* target = Head(), * source = sourceList.Head();
		RemoveAll();
		if (!source->data) return;
		target->data = source->data;
		while (source = source->next)
			target = target->Append(source->data);
	}

	template <class Op>
	void Visit(Op& op, Node* prev = NULL) const
	{
		Node* curr = prev ? prev->next : Head();
		while (curr)
		{
			if (!curr->data || !op.Accept(curr->data)) break;
			curr = curr->next;
		}
	}

	template <class Op>
	Item* Find(Op& op) const
	{
		Node* curr = Head();
		Item* pItem;
		do
		{
			pItem = curr->data;
			if (pItem && op.Accept(pItem)) return pItem;
			curr = curr->next;
		} while (curr);
		return NULL;
	}

	template <class Op>
	Iterator Find(Op& op, Iterator& prev) const
	{
		Iterator curIt = prev.End() ? Begin() : ++prev;
		while (!curIt.End())
		{
			if (*curIt && op.Accept(*curIt)) break;
			++curIt;
		}
		return curIt;
	}

	template <class Op>
	UInt32 CountIf(Op& op) const
	{
		UInt32 count = 0;
		Node* curr = Head();
		do
		{
			if (curr->data && op.Accept(curr->data)) count++;
			curr = curr->next;
		} while (curr);
		return count;
	}

	class AcceptAll
	{
	public:
		bool Accept(Item* item) { return true; }
	};

	void RemoveAll() const
	{
		Node* nextNode = Head(), * currNode = nextNode->next;
		nextNode->data = NULL;
		nextNode->next = NULL;
		while (currNode)
		{
			nextNode = currNode->next;
			FormHeap_Free(currNode);
			currNode = nextNode;
		}
	}

	void DeleteAll() const
	{
		Node* nextNode = Head(), * currNode = nextNode->next;
		FormHeap_Free(nextNode->data);
		nextNode->data = NULL;
		nextNode->next = NULL;
		while (currNode)
		{
			nextNode = currNode->next;
			FormHeap_Free(currNode->data);
			FormHeap_Free(currNode);
			currNode = nextNode;
		}
	}

	void DeleteHead() const
	{
		auto node = Head();
		if (node->data)
		{
			FormHeap_Free(node->data);
			node->data = NULL;
		}

		if (auto next = node->next)
		{
			node->data = next->data;
			node->next = next->next;
			FormHeap_Free(next);
		}
	}

	Item* RemoveNth(SInt32 idx)
	{
		Item* removed = NULL;
		if (idx <= 0)
		{
			removed = m_listHead.data;
			m_listHead.RemoveMe();
		}
		else
		{
			Node* node = Head();
			while (node->next && --idx)
				node = node->next;
			if (!idx)
			{
				removed = node->next->data;
				node->RemoveNext();
			}
		}
		return removed;
	};

	UInt32 Remove(Item* item)
	{
		UInt32 removed = 0;
		Node* curr = Head(), * prev = NULL;
		do
		{
			if (curr->data == item)
			{
				curr = prev ? prev->RemoveNext() : curr->RemoveMe();
				removed++;
			}
			else
			{
				prev = curr;
				curr = curr->next;
			}
		} while (curr);
		return removed;
	}

	void RemoveFirst(Item* item)
	{
		Node* curr = Head(), * prev = NULL;
		do
		{
			if (curr->data == item)
			{
				prev ? prev->RemoveNext() : curr->RemoveMe();
				return;
			}
			prev = curr;
		} while (curr = curr->next);
	}

	Item* ReplaceNth(SInt32 index, Item* item)
	{
		Item* replaced = NULL;
		if (item)
		{
			Node* node;
			if (eListEnd == index)
				node = GetLastNode();
			else
			{
				node = GetNthNode(index);
				if (!node) return NULL;
			}
			replaced = node->data;
			node->data = item;
		}
		return replaced;
	}

	UInt32 Replace(Item* item, Item* replace)
	{
		UInt32 replaced = 0;
		Node* curr = Head();
		do
		{
			if (curr->data == item)
			{
				curr->data = replace;
				replaced++;
			}
			curr = curr->next;
		} while (curr);
		return replaced;
	}

	template <class Op>
	UInt32 RemoveIf(Op& op)
	{
		return FreeNodes(Head(), op);
	}

	SInt32 GetIndexOf(Item* item)
	{
		SInt32 idx = 0;
		Node* curr = Head();
		do
		{
			if (curr->data == item) return idx;
			idx++;
			curr = curr->next;
		} while (curr);
		return -1;
	}

	template <class Op>
	SInt32 GetIndexOf(Op& op)
	{
		SInt32 idx = 0;
		Node* curr = Head();
		do
		{
			if (curr->data && op.Accept(curr->data)) return idx;
			idx++;
			curr = curr->next;
		} while (curr);
		return -1;
	}
};
STATIC_ASSERT(sizeof(tList<void*>) == 0x8);

// 010
template <class T>
class BSSimpleList
{
public:
	BSSimpleList<T>();
	~BSSimpleList<T>();

	void**		_vtbl;	// 000
	tList<T>	list;	// 004
};	// 00C
STATIC_ASSERT(sizeof(BSSimpleList<void *>) == 0xC);

//#if RUNTIME
//
//#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
//const UInt32 _NiTMap_Lookup = 0x00853130;
//#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
//const UInt32 _NiTMap_Lookup = 0x00844740;
//#else
//#error
//#endif
//
//#else
//const UInt32 _NiTMap_Lookup = 0;
//#endif

//// 10
//template <typename T_Key, typename T_Data>
//class NiTMapBase
//{
//public:
//	NiTMapBase();
//	virtual ~NiTMapBase();
//
//	virtual UInt32	CalcBucket(T_Key data);
//	virtual bool	CompareKey(T_Key lhs, T_Key rhs);
//	virtual void	Unk_03(void);
//	virtual void	Unk_04(void);
//	virtual void	AllocateNewData(void);
//	virtual void	FreeData(T_Data data);
//
//	DEFINE_MEMBER_FN_LONG(NiTMapBase, Lookup, bool, _NiTMap_Lookup, T_Key key, T_Data * dataOut);
//
//	struct Bucket
//	{
//		Bucket	* next;
//		T_Key	key;
//		T_Data	data;
//	};
//
//	// void		** vtbl				// 00
//	UInt32		m_numBuckets;		// 04
//	Bucket		** m_buckets;		// 08
//	UInt32		m_numItems;			// 0C
//};

//template <typename T>
//struct NiTArray
//{
//	void	* _vtbl;	// 00
//	T		* data;		// 04
//	UInt16	unk08;		// 08 - current size init'd to size of preallocation
//	UInt16	length;		// 0A - init'd to 0
//	UInt16	unk0C;		// 0C - count of not null elements init'd to 0
//	UInt16	unk0E;		// 0E - available size init'd to size of preallocation
//
//	T operator[](UInt32 idx) {
//		if (idx < length)
//			return data[idx];
//		return NULL;
//	}
//
//	T Get(UInt32 idx) { return (*this)[idx]; }
//};

template <typename T>
struct BSSimpleArray
{
	void	* _vtbl;		// 00
	T		* data;			// 04
	UInt32	size;			// 08
	UInt32	alloc;			// 0C

	// this only compiles for pointer types
	T operator[](UInt32 idx) { if (idx < size) 
		return data[idx]; 
	return NULL; }
};

//// this is a NiTPointerMap <UInt32, T_Data>
//// todo: generalize key
//template <typename T_Data>
//class NiTPointerMap
//{
//public:
//	NiTPointerMap();
//	virtual ~NiTPointerMap();
//
//	struct Entry
//	{
//		Entry	* next;
//		UInt32	key;
//		T_Data	* data;
//	};
//
//	// note: traverses in non-numerical order
//	class Iterator
//	{
//		friend NiTPointerMap;
//
//	public:
//		Iterator(NiTPointerMap * table, Entry * entry = NULL, UInt32 bucket = 0)
//			:m_table(table), m_entry(entry), m_bucket(bucket) { FindValid(); }
//		~Iterator() { }
//
//		T_Data *	Get(void);
//		UInt32		GetKey(void);
//		bool		Next(void);
//		bool		Done(void);
//
//	private:
//		void		FindValid(void);
//
//		NiTPointerMap	* m_table;
//		Entry		* m_entry;
//		UInt32		m_bucket;
//	};
//
//	virtual UInt32	CalculateBucket(UInt32 key);
//	virtual bool	CompareKey(UInt32 lhs, UInt32 rhs);
//	virtual void	Fn_03(void);
//	virtual void	Fn_04(void);
//	virtual void	Fn_05(void);
//	virtual void	Fn_06(void);
//
//	T_Data *	Lookup(UInt32 key);
//
//	UInt32	m_numBuckets;
//	Entry	** m_buckets;
//	UInt32	m_numItems;
//};

//template <typename T_Data>
//T_Data * NiTPointerMap <T_Data>::Lookup(UInt32 key)
//{
//	for(Entry * traverse = m_buckets[key % m_numBuckets]; traverse; traverse = traverse->next)
//		if(traverse->key == key)
//			return traverse->data;
//
//	return NULL;
//}
//
//template <typename T_Data>
//T_Data * NiTPointerMap <T_Data>::Iterator::Get(void)
//{
//	if(m_entry)
//		return m_entry->data;
//
//	return NULL;
//}
//
//template <typename T_Data>
//UInt32 NiTPointerMap <T_Data>::Iterator::GetKey(void)
//{
//	if(m_entry)
//		return m_entry->key;
//
//	return 0;
//}
//
//template <typename T_Data>
//bool NiTPointerMap <T_Data>::Iterator::Next(void)
//{
//	if(m_entry)
//		m_entry = m_entry->next;
//
//	while(!m_entry && (m_bucket < (m_table->m_numBuckets - 1)))
//	{
//		m_bucket++;
//
//		m_entry = m_table->m_buckets[m_bucket];
//	}
//
//	return m_entry != NULL;
//}
//
//template <typename T_Data>
//bool NiTPointerMap <T_Data>::Iterator::Done(void)
//{
//	return m_entry == NULL;
//}
//
//template <typename T_Data>
//void NiTPointerMap <T_Data>::Iterator::FindValid(void)
//{
//	// validate bucket
//	if(m_bucket >= m_table->m_numBuckets) return;
//
//	// get bucket
//	m_entry = m_table->m_buckets[m_bucket];
//
//	// find non-empty bucket
//	while(!m_entry && (m_bucket < (m_table->m_numBuckets - 1)))
//	{
//		m_bucket++;
//
//		m_entry = m_table->m_buckets[m_bucket];
//	}
//}

template <class Node, class Info>
class Visitor
{
	const Node* m_pHead;

	template <class Op>
	UInt32 FreeNodes(Node* node, Op &compareOp) const
	{
		static UInt32 nodeCount = 0;
		static UInt32 numFreed = 0;
		static Node* lastNode = NULL;
		static bool bRemovedNext = false;
		
		UInt32 returnCount;

		if (node->Next())
		{
			nodeCount++;
			FreeNodes(node->Next(), compareOp);
			nodeCount--;
		}

		if (compareOp.Accept(node->Info()))
		{
			if (nodeCount)
				node->Delete();
			else
				node->DeleteHead(lastNode);
			numFreed++;
			bRemovedNext = true;
		}
		else
		{
			if (bRemovedNext)
				node->SetNext(lastNode);
			bRemovedNext = false;
			lastNode = node;
		}

		returnCount = numFreed;

		if (!nodeCount)	//reset vars after recursing back to head
		{
			numFreed = 0;
			lastNode = NULL;
			bRemovedNext = false;
		}

		return returnCount;
	}

	class AcceptAll {
	public:
		bool Accept(Info* info) {
			return true;
		}
	};

	class AcceptEqual {
		const Info	* m_toMatch;
	public:
		AcceptEqual(const Info* info) : m_toMatch(info) { }
		bool Accept(const Info* info) {
			return info == m_toMatch;
		}
	};

	class AcceptStriCmp {
		const char * m_toMatch;
	public:
		AcceptStriCmp(const char* info) : m_toMatch(info) { }
		bool Accept(const char* info) {
			if (m_toMatch && info)
				return _stricmp(info, m_toMatch) ? false : true;
			return false;
		}
	};
public:
	Visitor(const Node* pHead) : m_pHead(pHead) { }
	
	UInt32 Count() const {
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur && pCur->Info() != NULL) {
			++count;
			pCur = pCur->Next();
		}
		return count;
	}

	Info* GetNthInfo(UInt32 n) const {
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur && count < n && pCur->Info() != NULL) {
			++count;
			pCur = pCur->Next();
		}
		return (count == n && pCur) ? pCur->Info() : NULL;
	}

	template <class Op>
	void Visit(Op& op) const {
		const Node* pCur = m_pHead;
		bool bContinue = true;
		while (pCur && pCur->Info() && bContinue) {
			bContinue = op.Accept(pCur->Info());
			if (bContinue) {
				pCur = pCur->Next();
			}
		}
	}

	template <class Op>
	const Node* Find(Op& op, const Node* prev = NULL) const 
	{
		const Node* pCur;
		if (!prev)
			pCur = m_pHead;
		else
			pCur = prev->next;
		bool bFound = false;
		while (pCur && !bFound)
		{
			if (!pCur->Info())
				pCur = pCur->Next();
			else
			{
				bFound = op.Accept(pCur->Info());
				if (!bFound)
					pCur = pCur->Next();
			}
		}

		return pCur;
	}

	Node* FindInfo(const Info* toMatch) {
		return Find(AcceptEqual(toMatch));
	}

#if 0
	const Node* FindString(char* str, const Node* prev = NULL) const
	{
		return Find(StringFinder_CI(str), prev);
	}
#endif

	template <class Op>
	UInt32 CountIf(Op& op) const
	{
		UInt32 count = 0;
		const Node* pCur = m_pHead;
		while (pCur)
		{
			if (pCur->Info() && op.Accept(pCur->Info()))
				count++;
			pCur = pCur->Next();
		}
		return count;
	}

	const Node* GetLastNode() const
	{
		const Node* pCur = m_pHead;
		while (pCur && pCur->Next())
			pCur = pCur->Next();
		return pCur;
	}

	void RemoveAll() const
	{
		FreeNodes(const_cast<Node*>(m_pHead), AcceptAll());
	}

	template <class Op>
	void RemoveIf(Op& op)
	{
		FreeNodes(const_cast<Node*>(m_pHead), op);
	}

	void Remove(const Info* toRemove)
	{
		RemoveIf(AcceptEqual(toRemove)) ? true : false;
	}

	void RemoveString(const char* toRemove)
	{
		RemoveIf(AcceptStriCmp(toRemove)) ? true : false;
	}

	void Append(Node* newNode)
	{
		Node* lastNode = const_cast<Node*>(GetLastNode());
		if (lastNode == m_pHead && !m_pHead->Info())
			lastNode->DeleteHead(newNode);
		else
			lastNode->SetNext(newNode);
	}

	template <class Op>
	UInt32 GetIndexOf(Op& op)
	{
		UInt32 idx = 0;
		const Node* pCur = m_pHead;
		while (pCur && pCur->Info() && !op.Accept(pCur->Info()))
		{
			idx++;
			pCur = pCur->Next();
		}

		if (pCur && pCur->Info())
			return idx;
		else
			return -1;
	}
};

template <typename T_Data> struct DListNode
{
public:
	DListNode* next;
	DListNode* prev;
	T_Data* data;

	DListNode* Advance(UInt32 times)
	{
		DListNode* result = this;
		while (result && times)
		{
			times--;
			result = result->next;
		}
		return result;
	}

	DListNode* Regress(UInt32 times)
	{
		DListNode* result = this;
		while (result && times)
		{
			times--;
			result = result->prev;
		}
		return result;
	}

	T_Data* GetAndAdvance()
	{
		T_Data* item = nullptr;
		if (next)
		{
			item = next->data;
			next = next->next;
		}
		return item;
	}

	T_Data* GetAndRegress()
	{
		T_Data* item = nullptr;
		if (next)
		{
			item = next->data;
			next = next->prev;
		}
		return item;
	}
};

template <class Item> class DList
{
public:
	typedef DListNode<Item> Node;

	// Nested iterator class
	class Iterator {
		Node* current;
	public:
		Iterator(Node* node) : current(node) {}

		Iterator& operator++() {
			current = current->next;
			return *this;
		}

		bool operator!=(const Iterator& other) const {
			return current != other.current;
		}

		Item* operator*() const {
			return current->data;
		}
	};

private:
	Node* first;
	Node* last;
	UInt32		count;

public:
	bool Empty() const { return !first; }
	Node* Head() { return first; }
	Node* Tail() { return last; }
	UInt32 Size() const { return count; }
	void SetHead(Node* head) { first = head; };
	void Init() { first = nullptr; last = nullptr; count = 0; };
	void Append(Item* item)
	{
		ThisCall(0x4ED8C0, this, &item);
	}
	Node* Remove(Item* item)
	{
		// return the item before the removed entry, or first if previous is null
		Node* result = nullptr;

		Node* node = Head();
		if (!node) return result;

		if (node->data == item)
		{
			first = node->next;
			if (first)
			{
				first->prev = nullptr;
			}
			else
			{
				last = nullptr;
			}

			--count;
			StdCall(0x4A49E0, node);
			node = first;
		}
		else
		{
			node = node->next;
		}

		if (node)
		{
			do
			{
				if (node->data == item)
				{
					result = node->prev;

					node->prev->next = node->next;
					if (node->next)
					{
						node->next->prev = node->prev;
					}
					else
					{
						last = node->prev;
					}
					--count;
					StdCall(0x4A49E0, node);
				}
			} while (node = node->next);
		}

		return result ? result : first;
	}

	void Sort(int (*compare)(Item* a, Item* b))
	{
		if (!first) return;
		Node* current = first;
		while (current->next)
		{
			Node* index = current->next;
			while (index)
			{
				if (compare(current->data, index->data) > 0)
				{
					Item* temp = current->data;
					current->data = index->data;
					index->data = temp;
				}
				index = index->next;
			}
			current = current->next;
		}
	}

	// begin and end functions for iterator support
	Iterator begin() {
		return Iterator(first);
	}

	Iterator end() {
		return Iterator(nullptr); // end iterator points to nullptr
	}
};