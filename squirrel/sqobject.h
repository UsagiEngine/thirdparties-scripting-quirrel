/*  see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include <type_traits>
#include <utility>

#include "squtils.h"

#define UINT32_MINUS_ONE (0xFFFFFFFF)

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct SQSharedState;

#define METAMETHODS_LIST \
    MM_IMPL(MT_ADD      ,"_add")\
    MM_IMPL(MT_SUB      ,"_sub")\
    MM_IMPL(MT_MUL      ,"_mul")\
    MM_IMPL(MT_DIV      ,"_div")\
    MM_IMPL(MT_UNM      ,"_unm")\
    MM_IMPL(MT_MODULO   ,"_modulo")\
    MM_IMPL(MT_SET      ,"_set")\
    MM_IMPL(MT_GET      ,"_get")\
    MM_IMPL(MT_TYPEOF   ,"_typeof")\
    MM_IMPL(MT_NEXTI    ,"_nexti")\
    MM_IMPL(MT_CMP      ,"_cmp")\
    MM_IMPL(MT_CALL     ,"_call")\
    MM_IMPL(MT_CLONED   ,"_cloned")\
    MM_IMPL(MT_NEWSLOT  ,"_newslot")\
    MM_IMPL(MT_DELSLOT  ,"_delslot")\
    MM_IMPL(MT_TOSTRING ,"_tostring")\
    MM_IMPL(MT_LOCK     ,"_lock")\


#define MM_IMPL(mm, name) mm,

enum SQMetaMethod{
    METAMETHODS_LIST
    MT_NUM_METHODS
};

#undef MM_IMPL

template <typename T>
void sq_unsafe_construct_vector_inplace(const SQInteger size, T *& ptr)
{
    using type = std::remove_cv_t<T>;
    for(SQInteger n = 0; n < size; n++)
    {
        new (&ptr[n]) type();
    }
}

template <typename T>
void sq_unsafe_destruct_vector_inplace(const SQInteger size, T *& ptr)
{
    using type = std::remove_cv_t<T>;
    for(SQInteger nl = 0; nl < size; nl++)
    {
        ptr[nl].~type();
    }
}

void sq_unsafe_copy_vector(auto && dest, auto && src, const SQInteger size)
{
    for(SQInteger _n_ = 0; _n_ < size; _n_++)
    {
        dest[_n_] = src[_n_];
    }
}

void sq_unsafe_nullify_vector_elements(auto && vec, const SQInteger size)
{
    for(SQInteger _n_ = 0; _n_ < size; _n_++)
    {
        vec[_n_].Null();
    }
}


struct SQRefCounted
{
    SQUnsignedInteger _uiRef;
    struct SQWeakRef *_weakref;
    SQRefCounted() { _uiRef = 0; _weakref = NULL; }
    virtual ~SQRefCounted();
    SQWeakRef *GetWeakRef(SQAllocContext alloc_ctx, SQObjectType type, SQObjectFlags flags);
    virtual void Release()=0;

};

struct SQWeakRef : SQRefCounted
{
    void Release();
    SQObject _obj;
    SQAllocContext _alloc_ctx;
};

struct SQObjectPtr;

inline void sq_try_add_ref(const SQObjectType type, SQObjectValue & unval)
{
    if(sq_is_ref_counted(type))
    {
        unval.pRefCounted->_uiRef++;
    }
}

inline void sq_try_release(const SQObjectType type, SQObjectValue & unval)
{
    if(sq_is_ref_counted(type))
    {
        auto & ref_count = unval.pRefCounted->_uiRef;
        --ref_count;
        assert(ref_count != (SQUnsignedInteger)-1);
        if(ref_count == 0) unval.pRefCounted->Release();
    }
}

template <typename RefCounted, typename T = std::remove_cv_t<RefCounted>>
    requires std::is_base_of_v<SQRefCounted, T>
void sq_object_release(RefCounted *& obj)
{
    if(obj)
    {
        auto & ref_count = obj->_uiRef;
        --ref_count;
        assert(ref_count != (SQUnsignedInteger)-1);
        if(ref_count == 0) obj->Release();
        obj = nullptr;
    }
}

inline void sq_object_add_ref(SQRefCounted * obj)
{
    assert(obj);
    obj->_uiRef++;
}

struct SQTable;
struct SQArray;
struct SQClosure;
struct SQOuter;
struct SQGenerator;
struct SQNativeClosure;
struct SQString;
struct SQUserData;
struct SQFunctionProto;
struct SQRefCounted;
struct SQDelegable;
struct SQVM;
struct SQClass;
struct SQInstance;
struct SQWeakRef;

// -----------------------------------------------------------------------------
// Helper Predicates
// -----------------------------------------------------------------------------

constexpr bool sq_is_delegable(auto && o)
{
    return sq_type(std::forward<decltype(o)>(o)) & SQOBJECT_DELEGABLE;
}

// -----------------------------------------------------------------------------
// Core Accessor Template (Replacing the _macro logic)
// -----------------------------------------------------------------------------

/* Shio: This template acts as the central dispatch for retrieving values
   from the union based on the compile-time Type constant.
   It uses std::forward to preserve value categories (L-value/R-value).
*/
template <SQObjectType Type>
constexpr auto & sq_check_cast_object(auto && o)
{
    if constexpr(Type == OT_INTEGER)
        return o._unVal.nInteger;
    else if constexpr(Type == OT_FLOAT)
        return o._unVal.fFloat;
    else if constexpr(Type == OT_STRING)
        return o._unVal.pString;
    else if constexpr(Type == OT_TABLE)
        return o._unVal.pTable;
    else if constexpr(Type == OT_ARRAY)
        return o._unVal.pArray;
    else if constexpr(Type == OT_CLOSURE)
        return o._unVal.pClosure;
    else if constexpr(Type == OT_GENERATOR)
        return o._unVal.pGenerator;
    else if constexpr(Type == OT_NATIVECLOSURE)
        return o._unVal.pNativeClosure;
    else if constexpr(Type == OT_USERDATA)
        return o._unVal.pUserData;
    else if constexpr(Type == OT_USERPOINTER)
        return o._unVal.pUserPointer;
    else if constexpr(Type == OT_THREAD)
        return o._unVal.pThread;
    else if constexpr(Type == OT_FUNCPROTO)
        return o._unVal.pFunctionProto;
    else if constexpr(Type == OT_CLASS)
        return o._unVal.pClass;
    else if constexpr(Type == OT_INSTANCE)
        return o._unVal.pInstance;
    else if constexpr(Type == OT_WEAKREF)
        return o._unVal.pWeakRef;
    else if constexpr(Type == OT_OUTER)
        return o._unVal.pOuter;
    else
        std::unreachable();
}

// -----------------------------------------------------------------------------
// Type-Safe Accessors
// -----------------------------------------------------------------------------

constexpr decltype(auto) sq_get_integer(auto && o)
{
    return sq_check_cast_object<OT_INTEGER>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_float(auto && o)
{
    return sq_check_cast_object<OT_FLOAT>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_string(auto && o)
{
    return sq_check_cast_object<OT_STRING>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_table(auto && o)
{
    return sq_check_cast_object<OT_TABLE>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_array(auto && o)
{
    return sq_check_cast_object<OT_ARRAY>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_closure(auto && o)
{
    return sq_check_cast_object<OT_CLOSURE>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_generator(auto && o)
{
    return sq_check_cast_object<OT_GENERATOR>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_nativeclosure(auto && o)
{
    return sq_check_cast_object<OT_NATIVECLOSURE>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_userdata(auto && o)
{
    return sq_check_cast_object<OT_USERDATA>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_userpointer(auto && o)
{
    return sq_check_cast_object<OT_USERPOINTER>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_thread(auto && o)
{
    return sq_check_cast_object<OT_THREAD>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_funcproto(auto && o)
{
    return sq_check_cast_object<OT_FUNCPROTO>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_class(auto && o)
{
    return sq_check_cast_object<OT_CLASS>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_instance(auto && o)
{
    return sq_check_cast_object<OT_INSTANCE>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_weakref(auto && o)
{
    return sq_check_cast_object<OT_WEAKREF>(std::forward<decltype(o)>(o));
}

constexpr decltype(auto) sq_get_outer(auto && o)
{
    return sq_check_cast_object<OT_OUTER>(std::forward<decltype(o)>(o));
}

// Special accessors for shared pointer types

// #define sq_get_delegable(obj) ((SQDelegable *)(obj)._unVal.pDelegable)
constexpr auto & sq_get_delegable(auto && o)
{
    // Replaces _delegable(obj)
    assert(sq_type(std::forward<decltype(o)>(o)) & SQOBJECT_DELEGABLE);
    return o._unVal.pDelegable;
}

constexpr auto & sq_get_refcounted(auto && o)
{
    // Replaces _refcounted(obj)
    assert(sq_type(std::forward<decltype(o)>(o)) & SQOBJECT_REF_COUNTED);
    return o._unVal.pRefCounted;
}

constexpr auto & sq_get_rawval(auto && o)
{
    // Replaces _rawval(obj)
    return o._unVal.raw;
}

// -----------------------------------------------------------------------------
// Numeric Conversions (tofloat/tointeger)
// -----------------------------------------------------------------------------

constexpr SQFloat sq_to_float(auto && o)
{
    // Logic: ((sq_type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
    if(sq_type(o) == OT_INTEGER)
    {
        return static_cast<SQFloat>(
            sq_get_integer(std::forward<decltype(o)>(o))
        );
    }
    return sq_get_float(std::forward<decltype(o)>(o));
}

constexpr SQInteger sq_to_integer(auto && o)
{
    // Logic: ((sq_type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
    if(sq_type(o) == OT_FLOAT)
    {
        return static_cast<SQInteger>(
            sq_get_float(std::forward<decltype(o)>(o))
        );
    }
    return sq_get_integer(std::forward<decltype(o)>(o));
}

// -----------------------------------------------------------------------------
// Structure Member Accessors (Requires full Type definitions)
// -----------------------------------------------------------------------------
/*
   Shio: The following functions replace `_stringval` and `_userdataval`.
   Note: These require `SQString` and `SQUserData` to be fully defined
   at the point of instantiation.
*/

// Replaces: _stringval(obj)
// For `stringval` don't return `decltype(auto)`.
// `_val` has to decay to `char *`.
constexpr auto & sq_get_stringval(auto && o)
{
    // return o._unVal.pString->_val;
    // Uncomment implementation when SQString definition is visible
    return sq_get_string(std::forward<decltype(o)>(o))->_val;
}

// Replaces: _userdataval(obj)
constexpr SQUserPointer sq_get_userdataval(auto && o)
{
    // Original: ((SQUserPointer)sq_aligning((obj)._unVal.pUserData + 1))
    // We treat the pointer math carefully here.
    const auto pUserData = sq_get_userdata(std::forward<decltype(o)>(o));
    // +1 on the struct pointer moves past the struct
    const auto raw_addr  = pUserData + 1;
    return reinterpret_cast<SQUserPointer>(sq_aligning(raw_addr));
}

// #define raw_type(obj) sq_get_raw_type((obj)._type)
constexpr auto sq_get_obj_raw_type(auto && o)
{
    return sq_get_raw_type(std::forward<decltype(o)>(o)._type);
}

constexpr SQObject sq_maybe_deref_weakptr(auto && o)
{
    return sq_type(std::forward<decltype(o)>(o)) != OT_WEAKREF
        ? (SQObject)o
        : sq_get_weakref(std::forward<decltype(o)>(o))->_obj;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
#define SQ_REFOBJECT_INIT() SQ_OBJECT_RAWINIT()
#else
#define SQ_REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
    explicit SQObjectPtr(_class * __restrict x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _flags=0; \
        _unVal.sym = x; \
        assert(_unVal.pTable); \
        _unVal.pRefCounted->_uiRef++; \
    } \
    inline SQObjectPtr& operator=(_class *__restrict x) \
    {  \
        SQObjectType  tOldType = _type; \
        SQObjectValue unOldVal = _unVal; \
        _type = type; \
        _flags = 0; \
        SQ_REFOBJECT_INIT() \
        _unVal.sym = x; \
        _unVal.pRefCounted->_uiRef++; \
        sq_try_release(tOldType,unOldVal); \
        return *this; \
    }

#define _SCALAR_TYPE_DECL(type,_class,sym) \
    explicit SQObjectPtr(_class x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _flags = 0; \
        _unVal.sym = x; \
    } \
    inline SQObjectPtr& operator=(_class x) \
    {  \
        sq_try_release(_type,_unVal); \
        _type = type; \
        _flags = 0; \
        SQ_OBJECT_RAWINIT() \
        _unVal.sym = x; \
        return *this; \
    }
struct SQObjectPtr : public SQObject
{
    SQObjectPtr() noexcept
    {
        memset(this, 0, sizeof(SQObjectPtr)); // OT_NULL == 0
    }
    SQObjectPtr(const SQObjectPtr &__restrict o)
    {
        memcpy(this, &o, sizeof(o));
        sq_try_add_ref(_type,_unVal);
    }
    SQObjectPtr(SQObjectPtr &&__restrict o) noexcept
    {
        memcpy(this, &o, sizeof(o));
        memset(&o, 0, sizeof(SQObjectPtr)); // OT_NULL == 0
    }
    explicit SQObjectPtr(const SQObject &__restrict o)
    {
        memcpy(this, &o, sizeof(o));
        sq_try_add_ref(_type,_unVal);
    }
    _REF_TYPE_DECL(OT_TABLE,SQTable,pTable)
    _REF_TYPE_DECL(OT_CLASS,SQClass,pClass)
    _REF_TYPE_DECL(OT_INSTANCE,SQInstance,pInstance)
    _REF_TYPE_DECL(OT_ARRAY,SQArray,pArray)
    _REF_TYPE_DECL(OT_CLOSURE,SQClosure,pClosure)
    _REF_TYPE_DECL(OT_NATIVECLOSURE,SQNativeClosure,pNativeClosure)
    _REF_TYPE_DECL(OT_OUTER,SQOuter,pOuter)
    _REF_TYPE_DECL(OT_GENERATOR,SQGenerator,pGenerator)
    _REF_TYPE_DECL(OT_STRING,SQString,pString)
    _REF_TYPE_DECL(OT_USERDATA,SQUserData,pUserData)
    _REF_TYPE_DECL(OT_WEAKREF,SQWeakRef,pWeakRef)
    _REF_TYPE_DECL(OT_THREAD,SQVM,pThread)
    _REF_TYPE_DECL(OT_FUNCPROTO,SQFunctionProto,pFunctionProto)

    _SCALAR_TYPE_DECL(OT_INTEGER,SQInteger,nInteger)
#ifdef _SQ64
    _SCALAR_TYPE_DECL(OT_INTEGER,SQInt32,nInteger)
#endif
    _SCALAR_TYPE_DECL(OT_FLOAT,SQFloat,fFloat)
    _SCALAR_TYPE_DECL(OT_USERPOINTER,SQUserPointer,pUserPointer)

    SQObjectPtr(SQVM *vm, const SQChar *str, SQInteger len = -1);

    explicit SQObjectPtr(bool bBool)
    {
        memset(this, 0, sizeof(SQObjectPtr));
        _type = OT_BOOL;
        if (bBool)
            _unVal.nInteger = 1;
    }
    inline SQObjectPtr& operator=(bool b)
    {
        sq_try_release(_type,_unVal);
        SQ_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _flags = 0;
        _unVal.nInteger = b?1:0;
        return *this;
    }

    ~SQObjectPtr()
    {
        sq_try_release(_type,_unVal);
    }

    inline SQObjectPtr& operator=(const SQObjectPtr& __restrict obj)
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal =_unVal;
        memcpy(this, &obj, sizeof(SQObjectPtr));
        sq_try_add_ref(_type,_unVal);
        sq_try_release(tOldType,unOldVal);
        return *this;
    }
    inline SQObjectPtr& operator=(const SQObject& __restrict obj)
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal =_unVal;
        memcpy(this, &obj, sizeof(SQObject));
        sq_try_add_ref(_type,_unVal);
        sq_try_release(tOldType,unOldVal);
        return *this;
    }
    inline SQObjectPtr& operator=(SQObjectPtr&& __restrict obj) noexcept
    {
        if (this != &obj) {
            sq_try_release(_type, _unVal);
            memcpy(this, &obj, sizeof(SQObjectPtr));
            memset(&obj, 0, sizeof(SQObjectPtr));  // OT_NULL == 0
        }
        return *this;
    }
    inline void Null()
    {
        SQObjectType  tOldType = _type;
        SQObjectValue unOldVal = _unVal;
        memset(this,0, sizeof(SQObjectPtr));
        _type = OT_NULL;
        sq_try_release(tOldType ,unOldVal);
    }
    private:
        SQObjectPtr(const SQChar *){} //safety
};


inline void _Swap(SQObject &a,SQObject &b)
{
    SQObject t = a;
    a = b;
    b = t;
}

/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
struct SQCollectable : public SQRefCounted {
    SQCollectable *_gc_next;
    SQCollectable *_gc_prev;
    SQSharedState *_sharedstate;
    virtual SQObjectType GetType()=0;
    virtual void Release()=0;
    virtual void Mark(SQCollectable **chain)=0;
    void UnMark();
    virtual void Finalize()=0;
    static void AddToChain(SQCollectable **chain,SQCollectable *c);
    static void RemoveFromChain(SQCollectable **chain,SQCollectable *c);
};

#define ADD_TO_CHAIN(chain, obj) AddToChain(chain, obj)
#define REMOVE_FROM_CHAIN(chain, obj)                          \
    do                                                         \
    {                                                          \
        if(!(_uiRef & MARK_FLAG)) RemoveFromChain(chain, obj); \
    }                                                          \
    while(0)
#define CHAINABLE_OBJ SQCollectable
#define INIT_CHAIN()         \
    do                       \
    {                        \
        _gc_next     = NULL; \
        _gc_prev     = NULL; \
        _sharedstate = ss;   \
    }                        \
    while(0)
#else

// Need this to keep SQSharedState pointer to access alloc_ctx
// Otherwise, just use SQRefCounted as CHAINABLE_OBJ in the way it was initially
struct SQRefCountedWithSharedState : public SQRefCounted
{
    SQSharedState *_sharedstate;
};
#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ SQRefCountedWithSharedState
#define INIT_CHAIN() {_sharedstate=ss;}

#endif

struct SQDelegable : public CHAINABLE_OBJ {
    bool SetDelegate(SQTable *m);
    virtual bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);
    SQTable *_delegate;
};

inline SQUnsignedInteger TranslateIndex(const SQObjectPtr &idx)
{
    switch(sq_type(idx)){
        case OT_NULL:
            return 0;
        case OT_INTEGER:
            return (SQUnsignedInteger)sq_get_integer(idx);
        default: assert(0); break;
    }
    return 0;
}

typedef sqvector<SQObjectPtr> SQObjectPtrVec;
typedef sqvector<SQInteger> SQIntVec;
const SQChar *GetTypeName(const SQObject &obj1);
const SQChar *IdType2Name(SQObjectType type);

#endif //_SQOBJECT_H_
