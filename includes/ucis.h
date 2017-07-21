/*******************************************************************************
 *
 * Copyright© 2012 by Accellera Systems Initiative All rights reserved.
 *
 ******************************************************************************/


/*-----------------------------------------------------------------------------
 *  General API Behavior - please read
 *
 *  The following notes describe default API behavior which may be assumed unless
 *  the description of a particular function notes otherwise.
 *
 *  Functions that return a handle will return NULL (or invalid handle) 
 *  on error, the handle otherwise. Search functions returning handles 
 *  return NULL if the object was not found; this is not an error and will 
 *  not trigger the error reporting subsystem.
 *
 *  Functions that return a status int will return non-zero on error, 0 otherwise. 
 *
 *  Functions that return strings guarantee only to keep the return string valid
 *  until the next call to the function, or the database is closed, whichever
 *  comes first. Applications must make their own copy of any string data 
 *  needed after this.
 *
 *  The iteration routines may cause memory to be allocated to hold the iteration 
 *  state. The ucis_FreeIterator() routine must be called when the caller is
 *  finished with the iteration, to release any memory so allocated.
 *  The iterator object is invalid after this call and must not be used again, even
 *  if an iteration was terminated before all possible objects had been returned.
 *  
 *  Where number sets are used as identifiers, all numbers below 1000 are
 *  reserved for future UCIS standardization extensions unless noted.
 *  This rule applies to both enumerated and #define definitions.
 *  
 *  In some routines, the combination of a scope handle and a coverindex is
 *  used to identify either a scope or a coveritem associated with the scope
 *  Unless noted, the following rules apply to these routines:
 *  - a NULL scope pointer implies the top level database scope
 *  - a coverindex value of -1 identifies the scope 
 *  - a coverindex value 0 or greater identifies a coveritem
 *
 *  Removal routines immediately invalidate the handle to the removed object
 *  Removal of a scope (in-memory) removes all the children of the scope
 *  
 * ---------------------------------------------------------------------------*/

#ifndef UCIS_API_H
#define UCIS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Ensure that size-critical types are defined on all OS platforms. */
#if defined (_MSC_VER)
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef signed __int64 int64_t;
typedef signed __int32 int32_t;
typedef signed __int16 int16_t;
typedef signed __int8 int8_t;
#elif defined(__MINGW32__)
#include <stdint.h>
#elif defined(__linux)
#include <inttypes.h>
#else
#include <sys/types.h>
#endif

/*
 * Macros to control literal size warnings cross-compiler
 */

#ifdef WIN32
#define INT64_LITERAL(val)     ((int64_t)val)
#define INT64_ZERO             ((int64_t)0)
#define INT64_ONE              ((int64_t)1)
#define INT64_NEG1             ((int64_t)-1)
#else
#define INT64_LITERAL(val)     (val##LL)
#define INT64_ZERO             (0LL)
#define INT64_ONE              (1LL)
#define INT64_NEG1             (-1LL)
#endif

/*
 * 64-bit one-hot typing
 */

typedef uint64_t ucisObjTypeT;
typedef ucisObjTypeT ucisScopeTypeT;
typedef ucisObjTypeT ucisCoverTypeT;
typedef uint64_t ucisCoverMaskTypeT;
typedef uint64_t ucisScopeMaskTypeT;



#ifndef DEFINE_UCIST
#define DEFINE_UCIST
typedef void* ucisT;          /* generic handle to a UCIS */
#endif

typedef void* ucisScopeT;       /* scope handle */
typedef void* ucisObjT;         /* either ucisScopeT or ucisHistoryNodeT */
typedef void* ucisFileHandleT;  /* file handle */
typedef void* ucisIteratorT;    /* opaque iterator object */
typedef void* ucisHistoryNodeT;

/*------------------------------------------------------------------------------
 *  Source information management
 *
 *----------------------------------------------------------------------------*/

typedef struct {
    ucisFileHandleT     filehandle;
    int                 line;
    int                 token;
} ucisSourceInfoT;

/*
 *  ucis_GetFileName()
 *  Given a file handle, return the actual file name.
 *
 */
const char*
ucis_GetFileName(
    ucisT               db,
    ucisFileHandleT     filehandle);

/*
 *  ucis_CreateFileHandle()
 *  Create and return a file handle.
 *  When the "filename" is absolute, "fileworkdir" is ignored.
 *  When the "filename" is relative, it is desirable to set "fileworkdir"
 *  to the directory path which it is relative to.
 *  Returns NULL for error.
 */
ucisFileHandleT
ucis_CreateFileHandle (
    ucisT db,
    const char* filename,
    const char* fileworkdir);

/*------------------------------------------------------------------------------
 *  Error-handling
 *
 *  Optionally use ucis_RegisterErrorHandler() before any UCIS calls.
 *  The user's error callback, a function pointer of type ucis_ErrorHandler,
 *  is called for any errors produced by the system.
 *
 *----------------------------------------------------------------------------*/
typedef enum {
    UCIS_MSG_INFO,
    UCIS_MSG_WARNING,
    UCIS_MSG_ERROR
} ucisMsgSeverityT;

typedef struct ucisErr_s {
    int msgno;                   /* Message identifier */
    ucisMsgSeverityT severity;   /* Message severity */
    const char* msgstr;          /* Raw message string */
} ucisErrorT;

typedef void (*ucis_ErrorHandler) (void* userdata, ucisErrorT* errdata);

/*
 *  ucis_RegisterErrorHandler()
 *  The registered function, if set, will be called for all API errors.
 */
void
ucis_RegisterErrorHandler(
    ucis_ErrorHandler errHandle,
    void*             userdata);

/*------------------------------------------------------------------------------
 *  Database creation and file management
 *
 *  Notes on callback order:
 *
 *      * INITDB is always the first callback.
 *      * all TEST callbacks follow; after the next non-TEST callback
 *        there will be no more TEST callbacks.
 *      * DU callbacks must precede their first associated instance SCOPE
 *        callbacks, but need not immediately precede them.
 *      * SCOPE and DU and CVBIN callbacks can occur in any order
 *        (excepting the DU before first instance rule) -- though
 *        nesting level is implied by the order of callbacks.
 *      * ENDSCOPE callbacks correspond to SCOPE and DU callbacks and
 *        imply a "pop" in the nesting of scopes and design units.
 *      * ENDDB callbacks can be used to access UCIS attributes written at the
 *        end of the file, if created in write streaming modes.
 *
 *----------------------------------------------------------------------------*/

/*
 *  ucis_Open()
 *  Create an in-memory database, optionally populating it from the given file.
 *  Returns a handle with success, NULL with failure.
 */
ucisT
ucis_Open(
    const char*     name        /* file system path */
);

/*
 * ucis_OpenFromInterchangeFormat()
 * Create an in-memory database, optionally populating it from the given
 * interchange format file.
 * Returns a handle with success, NULL with failure.
 */

ucisT
ucis_OpenFromInterchangeFormat(
    const char* name /* file system path */
);

/*
 *  Opening UCIS in streaming mode to read data through callbacks without
 *  creating an in-memory database.
 */

/* Enum type for different callback reasons */
typedef enum {
    UCIS_REASON_INITDB,      /* Start of the database, apply initial settings */
    UCIS_REASON_DU,          /* Start of a design unit scope */
    UCIS_REASON_TEST,        /* Testdata history object */
    UCIS_REASON_SCOPE,       /* Start of a scope object */
    UCIS_REASON_CVBIN,       /* Cover item */
    UCIS_REASON_ENDSCOPE,    /* End of a scope, including design units */
    UCIS_REASON_ENDDB,       /* End of database (database handle still valid) */
    UCIS_REASON_MERGEHISTORY /* Merge history object */
} ucisCBReasonT;


/* Enum type for return type for of ucis_CBFuncT function */
typedef enum {
    UCIS_SCAN_CONTINUE = -1,    /* Continue to scan ucis file */
    UCIS_SCAN_STOP     = -2,    /* Stop scanning ucis file */
    UCIS_SCAN_PRUNE    = -3     /* Prune the scanning of the ucis file at this
                                 * node. Scanning continues but ignores
                                 * processing of this node's children.
                                 * NOTE: This enum value is currently
                                 *       disallowed in read streaming mode.
                                 */
} ucisCBReturnT;

/* Data type for read callback */
typedef struct ucisCBDataS {
    ucisCBReasonT reason;       /* Reason for this callback */
    ucisT         db;           /* Database handle, to use in APIs */
    ucisObjT      obj;          /* ucisScopeT or ucisHistoryNodeT */
    int           coverindex;   /* if UCIS_REASON_CVBIN, index of coveritem */
} ucisCBDataT;

/* Function type to use in the ucis_OpenReadStream() read API */
typedef ucisCBReturnT (*ucis_CBFuncT) (void* userdata, ucisCBDataT* cbdata);

/*
 *  ucis_OpenReadStream()
 *  Open database for streaming read mode.
 *  Returns 0 with success, -1 with failure.
 */
int
ucis_OpenReadStream(
    const char*  name,      /* file system path */
    ucis_CBFuncT cbfunc,
    void*        userdata);


/*
 *  ucis_OpenWriteStream()
 *  Open data in write streaming mode.
 *  Implementations may impose ordering restrictions in write-streaming mode.
 *  Returns a restricted database handle with success, NULL with error.
 */
ucisT
ucis_OpenWriteStream(
    const char* name);      /* file system path */

/*
 *  ucis_WriteStream()
 *  Finishes a write of current object to the persistent database file in
 *  write streaming mode.
 *  Note: this operation is like a "flush", just completing the write of
 *  whatever was most recently created in write streaming mode.  In particular,
 *  there is no harm in doing multiple ucis_WriteStream() calls, because if the
 *  current object has already been written, it will not be written again.
 *  The database handle "db" must have been previously opened with
 *  ucis_OpenWriteStream().
 *  UCIS_WRITESTREAMING mode. Returns 0 for success, and -1 for any error.
 */
int
ucis_WriteStream(
    ucisT       db);

/*
 *  ucis_WriteStreamScope()
 *  Similar to ucis_WriteStream, except that it finishes the current scope and
 *  "pops" the stream to the parent scope.  Objects created after this belong
 *  to the parent scope of the scope just ended.
 *  The database handle "db" must have been previously opened with
 *  ucis_OpenWriteStream().
 *  Returns 0 for success, and -1 for any error.
 */
int
ucis_WriteStreamScope(
    ucisT       db);

/*
 *  ucis_Write()
 *  Copy in-memory database or a subset of the in-memory database to a
 *  persistent form stored in the given file name.
 *  Use the coverflags argument to select coverage types to be saved, use -1
 *  to save everything.
 *  Note that file system path will be overwritten if it contains existing
 *  data; write permissions must exist.
 *  The database handle "db" cannot have been opened for one of the streaming
 *  modes.
 *  Returns 0 for success, and -1 for any error.
 */
int
ucis_Write(
    ucisT       db,
    const char* file,
    ucisScopeT  scope,      /* write objects under given scope, write all
                               objects if scope==NULL */
    int         recurse,    /* if true, recurse under given scope, ignored if
                               scope==NULL */
    /* ucisCoverTypeT: */
    int         covertype); /* selects coverage types to save */

/*
 * ucis_WriteToInterchangeFormat()
 * Copy in-memory database or a subset of the in-memory database to a
 * persistent form stored in the given file name in the interchange format.
 * Use the coverflags argument to select coverage types to be saved, use -1
 * to save everything.
 * Note that file system path will be overwritten if it contains existing
 * data; write permissions must exist.
 * The database handle "db" cannot have been opened for one of the
 * streaming modes.
 * Returns 0 for success, and -1 for any error.
 */
int
ucis_WriteToInterchangeFormat(
    ucisT db,
    const char* file,
    ucisScopeT scope, /* write objects under given scope, write all 
                         objects if scope==NULL */
    int recurse,      /* if true, recurse under given scope, ignored if
                         ucisCoverTypeT: */
    int covertype);   /* selects coverage types to save */

/*
 *  ucis_Close()
 *  Invalidate the database handle.  Frees all memory associated with the
 *  database handle, including the in-memory image of the database if not in
 *  one of the streaming modes.  If opened with ucis_OpenWriteStream(), this
 *  also has the side-effect of closing the output file.
 *  Returns 0 with success, non-zero with failure.
 */
int
ucis_Close(
    ucisT       db);

/*
 *  ucis_SetPathSeparator()
 *  ucis_GetPathSeparator()
 *  Set or get path separator used with the UCIS.
 *  The path separator is associated with the database, different databases may
 *  have different path separators; the path separator is stored with the
 *  persistent form of the database.
 *  Both routines return -1 with error, SetPathSeparator returns 0 if OK.
 */
int
ucis_SetPathSeparator(
    ucisT           db,
    char            separator);

char
ucis_GetPathSeparator(
    ucisT           db);


/*------------------------------------------------------------------------------
 *  Property APIs
 *  
 *  The property routines provide access to pre-defined properties associated
 *  with database objects.
 *  Some properties may be read-only, for example the UCIS_STR_UNIQUE_ID property
 *  may be queried but not set. An attempt to set a read-only property will
 *  result in an error.
 *  Some properties may be in-memory only, for example the UCIS_INT_IS_MODIFIED
 *  property applies only to an in-memory database and indicates that the
 *  data has been modified since it was last saved. This property is not 
 *  stored in the database.
 *  See also the attribute routines for extended data management
 *----------------------------------------------------------------------------*/ 

/*------------------------------------------------------------------------------
 *  Integer Properties
 *----------------------------------------------------------------------------*/ 

typedef enum {
    UCIS_INT_IS_MODIFIED,	/* Modified since opening stored UCISDB (In-memory and read only) */
    UCIS_INT_MODIFIED_SINCE_SIM,	/* Modified since end of simulation run (In-memory and read only) */
    UCIS_INT_NUM_TESTS,	/* Number of test history nodes (UCIS_HISTORYNODE_TEST) in UCISDB */
    UCIS_INT_SCOPE_WEIGHT,	/* Scope weight */
    UCIS_INT_SCOPE_GOAL,	/* Scope goal */
    UCIS_INT_SCOPE_SOURCE_TYPE,	/* Scope source type (ucisSourceT) */
    UCIS_INT_NUM_CROSSED_CVPS,	/* Number of coverpoints in a cross (read only) */
    UCIS_INT_SCOPE_IS_UNDER_DU,	/* Scope is underneath design unit scope (read only) */
    UCIS_INT_SCOPE_IS_UNDER_COVERINSTANCE,	/* Scope is underneath covergroup instance (read only) */
    UCIS_INT_SCOPE_NUM_COVERITEMS,	/* Number of coveritems underneath scope (read only) */
    UCIS_INT_SCOPE_NUM_EXPR_TERMS,	/* Number of input ordered expr term strings delimited by '#' */
    UCIS_INT_TOGGLE_TYPE,	/* Toggle type (ucisToggleTypeT) */
    UCIS_INT_TOGGLE_DIR,	/* Toggle direction (ucisToggleDirT) */
    UCIS_INT_TOGGLE_COVERED,	/* Toggle object is covered */
    UCIS_INT_BRANCH_HAS_ELSE,	/* Branch has an 'else' coveritem */
    UCIS_INT_BRANCH_ISCASE,	/* Branch represents 'case' statement */
    UCIS_INT_COVER_GOAL,	/* Coveritem goal */
    UCIS_INT_COVER_LIMIT,	/* Coverage count limit for coveritem */
    UCIS_INT_COVER_WEIGHT,	/* Coveritem weight */
    UCIS_INT_TEST_STATUS,	/* Test run status (ucisTestStatusT) */
    UCIS_INT_TEST_COMPULSORY,	/* Test run is compulsory */
    UCIS_INT_STMT_INDEX,	/* Index or number of statement on a line */
    UCIS_INT_BRANCH_COUNT,	/* Total branch execution count */
    UCIS_INT_FSM_STATEVAL,	/* FSM state value */
    UCIS_INT_CVG_ATLEAST,	/* Covergroup at_least option */
    UCIS_INT_CVG_AUTOBINMAX,	/* Covergroup auto_bin_max option */
    UCIS_INT_CVG_DETECTOVERLAP,	/* Covergroup detect_overlap option */
    UCIS_INT_CVG_NUMPRINTMISSING,	/* Covergroup cross_num_print_missing option */
    UCIS_INT_CVG_STROBE,	/* Covergroup strobe option */
    UCIS_INT_CVG_PERINSTANCE,	/* Covergroup per_instance option */
    UCIS_INT_CVG_GETINSTCOV,	/* Covergroup get_inst_coverage option */
    UCIS_INT_CVG_MERGEINSTANCES	/* Covergroup merge_instances option */
} ucisIntPropertyEnumT;

int
ucis_GetIntProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisIntPropertyEnumT    property);

int
ucis_SetIntProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisIntPropertyEnumT    property,
    int                     value);

/*------------------------------------------------------------------------------
 *  String Properties
 *----------------------------------------------------------------------------*/ 
typedef enum {
    UCIS_STR_FILE_NAME,	/* UCISDB file/directory name (read only) */
    UCIS_STR_SCOPE_NAME,	/* Scope name */
    UCIS_STR_SCOPE_HIER_NAME,	/* Hierarchical scope name */
    UCIS_STR_INSTANCE_DU_NAME,	/* Instance' design unit name */
    UCIS_STR_UNIQUE_ID,	/* Scope or coveritem unique-id (read only) */
    UCIS_STR_VER_STANDARD,	/* Standard (Currently fixed to be "UCIS") */
    UCIS_STR_VER_STANDARD_VERSION,	/* Version of standard (e.g. "2011", etc) */
    UCIS_STR_VER_VENDOR_ID,	/* Vendor id (e.g. "CDNS", "MENT", "SNPS", etc) */
    UCIS_STR_VER_VENDOR_TOOL,	/* Vendor tool (e.g. "Incisive", "Questa", "VCS", etc) */
    UCIS_STR_VER_VENDOR_VERSION,	/* Vendor tool version (e.g. "6.5c", "Jun-12-2009", etc) */
    UCIS_STR_GENERIC,	/* Miscellaneous string data */
    UCIS_STR_ITH_CROSSED_CVP_NAME,	/* Ith coverpoint name of a cross */
    UCIS_STR_HIST_CMDLINE,	/* Test run command line */
    UCIS_STR_HIST_RUNCWD,	/* Test run working directory */
    UCIS_STR_COMMENT,	/* Comment */
    UCIS_STR_TEST_TIMEUNIT,	/* Test run simulation time unit */
    UCIS_STR_TEST_DATE,	/* Test run date */
    UCIS_STR_TEST_SIMARGS,	/* Test run simulator arguments */
    UCIS_STR_TEST_USERNAME,	/* Test run user name */
    UCIS_STR_TEST_NAME,	/* Test run name */
    UCIS_STR_TEST_SEED,	/* Test run seed */
    UCIS_STR_TEST_HOSTNAME,	/* Test run hostname */
    UCIS_STR_TEST_HOSTOS,	/* Test run hostOS */
    UCIS_STR_EXPR_TERMS,	/* Input ordered expr term strings delimited by '#' */
    UCIS_STR_TOGGLE_CANON_NAME,	/* Toggle object canonical name */
    UCIS_STR_UNIQUE_ID_ALIAS,	/* Scope or coveritem unique-id alias */
    UCIS_STR_DESIGN_VERSION_ID,	/* Version of the design or elaboration-id */
    UCIS_STR_DU_SIGNATURE,
    UCIS_STR_HIST_TOOLCATEGORY,
    UCIS_STR_HIST_LOG_NAME,
    UCIS_STR_HIST_PHYS_NAME
} ucisStringPropertyEnumT;


const char*
ucis_GetStringProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisStringPropertyEnumT property);

int
ucis_SetStringProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisStringPropertyEnumT property,
    const char*             value);


/*------------------------------------------------------------------------------
 *  Real-valued Properties
 *----------------------------------------------------------------------------*/ 
typedef enum {
    UCIS_REAL_HIST_CPUTIME,	/* Test run CPU time */
    UCIS_REAL_TEST_SIMTIME,	/* Test run simulation time */
    UCIS_REAL_TEST_COST,
    UCIS_REAL_CVG_INST_AVERAGE
} ucisRealPropertyEnumT;


double
ucis_GetRealProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisRealPropertyEnumT   property);

int
ucis_SetRealProperty(
    ucisT                   db,
    ucisObjT                obj,
    int                     coverindex,
    ucisRealPropertyEnumT   property,
    double                  value);


/*------------------------------------------------------------------------------
 *  Object Handle Properties
 *----------------------------------------------------------------------------*/ 

typedef enum {
    UCIS_HANDLE_SCOPE_PARENT,	/* Parent scope */
    UCIS_HANDLE_SCOPE_TOP,	/* Top (root) scope */
    UCIS_HANDLE_INSTANCE_DU,	/* Instance' design unit scope */
    UCIS_HANDLE_HIST_NODE_PARENT,	/* Parent history node */
    UCIS_HANDLE_HIST_NODE_ROOT	/* Top (root) history node */
} ucisHandleEnumT;

ucisObjT
ucis_GetHandleProperty(
    ucisT                   db,
    ucisObjT                obj,       /* scope or history node */
    ucisHandleEnumT         property);

int
ucis_SetHandleProperty(
    ucisT                   db,
    ucisObjT                obj,       /* scope or history node */
    ucisHandleEnumT         property,
    ucisObjT                value);


/*------------------------------------------------------------------------------
 *  Version query API
 *
 *----------------------------------------------------------------------------*/

typedef void* ucisVersionHandleT;


ucisVersionHandleT ucis_GetAPIVersion();
ucisVersionHandleT ucis_GetDBVersion(ucisT db);
ucisVersionHandleT ucis_GetFileVersion(char* filename_or_directory_name);
ucisVersionHandleT ucis_GetHistoryNodeVersion(ucisT db, ucisHistoryNodeT hnode);

const char* 
ucis_GetVersionStringProperty (
    ucisVersionHandleT versionH,
    ucisStringPropertyEnumT property);


/*------------------------------------------------------------------------------
 *  Traversal API
 *
 *  These routines provide access to the database history node, scope and 
 *  coveritem objects
 *  Iteration and callback mechanisms provide for multiple object returns.
 *  The iteration routines are only available for objects that are
 *  currently in memory. 
 *  The callback mechanism is available for both in-memory and 
 *  read-streaming applications (see the ucis_OpenReadStream() routine).
 *  
 *  Objects may also be found from in-memory scope hierarchies by matching
 *  the Unique ID.
 *  These routines return 0 or 1 objects.
 *  
 *  Generally, returned items are of known object kind (e.g. scopes, coveritems,
 *  history nodes), but the ucis_TaggedObjIterate()/ucis_TaggedObjScan mechanism 
 *  may return mixed handles, i.e. a mix of history nodes, scopes, and coveritems.
 *  The ucis_ObjKind() routine may be used to indicate the typing scheme of the
 *  returned handle.
 *  
 *----------------------------------------------------------------------------*/


/*
 * Enum type for different object kinds. This is a bit mask for the different
 * kinds of objects which may be tagged. Mask values may be and'ed and or'ed
 * together.
 */
typedef enum {
    UCIS_OBJ_ERROR       = 0x00000000,    /* Start of the database, apply initial settings */
    UCIS_OBJ_HISTORYNODE = 0x00000001,    /* History node object */
    UCIS_OBJ_SCOPE       = 0x00000002,    /* Scope object */
    UCIS_OBJ_COVERITEM   = 0x00000004,    /* Coveritem object */
    UCIS_OBJ_ANY         = -1             /* All taggable types */
} ucisObjMaskT;


/*
 *  ucis_ObjKind()
 *  Given 'obj' return the kind of object that it is.
 */

ucisObjMaskT
ucis_ObjKind(ucisT db, ucisObjT obj);

ucisIteratorT
ucis_ScopeIterate(
    ucisT               db,
    ucisScopeT          scope,
    ucisScopeMaskTypeT  scopemask);

ucisScopeT
ucis_ScopeScan(
    ucisT               db,
    ucisIteratorT       iterator);

void
ucis_FreeIterator(
    ucisT               db,
    ucisIteratorT       iterator);

ucisIteratorT
ucis_CoverIterate(
    ucisT               db,
    ucisScopeT          scope,
    ucisCoverMaskTypeT  covermask);

int
ucis_CoverScan(
    ucisT               db,
    ucisIteratorT       iterator);

ucisIteratorT
ucis_TaggedObjIterate(
    ucisT               db,
    const char*         tagname);

ucisObjT
ucis_TaggedObjScan(
    ucisT               db,
    ucisIteratorT       iterator,
	int*				coverindex_p);

ucisIteratorT
ucis_ObjectTagsIterate(
    ucisT               db,
    ucisObjT            obj,
    int                 coverindex);

const char*
ucis_ObjectTagsScan(
    ucisT               db,
    ucisIteratorT       iterator);

ucisScopeT
ucis_MatchScopeByUniqueID(
    ucisT db,
    ucisScopeT scope,
    const char *uniqueID);

ucisScopeT
ucis_CaseAwareMatchScopeByUniqueID(
    ucisT db,
    ucisScopeT scope,
    const char *uniqueID);

/*
 * ucis_MatchCoverByUniqueID() and ucis_CaseAwareMatchCoverByUniqueID()
 * 
 * A coveritem is defined by a combination of parental
 * scope (the returned handle) plus coverindex (returned in
 * the index pointer)
 */
ucisScopeT
ucis_MatchCoverByUniqueID(
    ucisT db,
    ucisScopeT scope,
    const char *uniqueID,
    int *index);

ucisScopeT
ucis_CaseAwareMatchCoverByUniqueID(
    ucisT db,
    ucisScopeT scope,
    const char *uniqueID,
    int *index);

/*
 *  ucis_CallBack()
 *  Visit that portion of the database rooted at and below the given starting
 *  scope. Issue calls to the callback function (cbfunc) along the way. When
 *  the starting scope (start) is NULL the entire database will be walked.
 *  Returns 0 with success, -1 with failure.
 *  In-memory mode only.
 */

int
ucis_CallBack(
    ucisT        db,
    ucisScopeT   start,             /* NULL traverses entire database */
    ucis_CBFuncT cbfunc,
    void*        userdata);

/*------------------------------------------------------------------------------
 *  Attributes (key/value pairs)
 *  Attribute key names shall be unique for the parental object
 *  Adding an attribute with a key name that already exists shall overwrite
 *  the existing attribute value.
 *----------------------------------------------------------------------------*/

typedef enum {
    UCIS_ATTR_INT,
    UCIS_ATTR_FLOAT,
    UCIS_ATTR_DOUBLE,
    UCIS_ATTR_STRING,
    UCIS_ATTR_MEMBLK,
    UCIS_ATTR_INT64
} ucisAttrTypeT;

typedef struct {
    ucisAttrTypeT type;     /* Value type */
    union {
        int64_t i64value;   /* 64-bit integer value */
        int ivalue;         /* Integer value */
        float fvalue;       /* Float value */
        double dvalue;      /* Double value */
        const char* svalue; /* String value */
        struct {
            int size;            /* Size of memory block, number of bytes */
            unsigned char* data; /* Starting address of memory block */
        } mvalue;
    } u;
} ucisAttrValueT;

const char*
ucis_AttrNext(
    ucisT           db,
    ucisObjT        obj,        /* ucisScopeT, ucisHistoryNodeT, or NULL */
    int             coverindex, /* obj is ucisScopeT: -1 for scope, valid index
                                   for coveritem */
    const char*     key,        /* NULL to get the first one */
    ucisAttrValueT**value);

int
ucis_AttrAdd(
    ucisT           db,
    ucisObjT        obj,        /* ucisScopeT, ucisHistoryNodeT, or NULL */
    int             coverindex, /* obj is ucisScopeT: -1 for scope, valid index
                                   for coveritem */
    const char*     key,        /* New attribute key */
    ucisAttrValueT* value);     /* New attribute value */

int
ucis_AttrRemove(
    ucisT           db,
    ucisObjT        obj,        /* ucisScopeT, ucisHistoryNodeT, or NULL */
    int             coverindex, /* obj is ucisScopeT: -1 for scope, valid index
                                   for coveritem */
    const char*     key);       /* NULL to get the first one */

ucisAttrValueT*
ucis_AttrMatch(
    ucisT           db,
    ucisObjT        obj,        /* ucisScopeT, ucisHistoryNodeT, or NULL */
    int             coverindex, /* obj is ucisScopeT: -1 for scope, valid index
                                   for coveritem */
    const char*     key);

/*------------------------------------------------------------------------------
 *  History Nodes
 *
 *  History nodes are attribute collections that record the historical
 *  construction process for the database
 *----------------------------------------------------------------------------*/

typedef int   ucisHistoryNodeKindT; /* takes the #define'd types below */

/*
 * History node kinds.
 */
#define UCIS_HISTORYNODE_NONE    -1  /* no node or error */
#define UCIS_HISTORYNODE_ALL      0  /* valid only in iterate-all request */
                                     /* (no real object gets this value)  */
#define UCIS_HISTORYNODE_TEST     1  /* test leaf node (primary database) */
#define UCIS_HISTORYNODE_MERGE    2  /* merge node */

/*
 * Pre-defined tool category attribute strings
 */
#define UCIS_SIM_TOOL        "UCIS:Simulator"
#define UCIS_FORMAL_TOOL     "UCIS:Formal"
#define UCIS_ANALOG_TOOL     "UCIS:Analog"
#define UCIS_EMULATOR_TOOL   "UCIS:Emulator"
#define UCIS_MERGE_TOOL      "UCIS:Merge"


/*
 *  ucis_CreateHistoryNode()
 *  Create a HistoryNode handle of the indicated kind
 *  linked into the database db.
 *  Returns NULL for error or the history node already exists.
 *
 */
ucisHistoryNodeT
ucis_CreateHistoryNode(
    ucisT                    db,
    ucisHistoryNodeT         parent,
    char*                    logicalname,   /* primary key, never NULL */
    char*                    physicalname,
    ucisHistoryNodeKindT     kind);         /* takes predefined values, above */

/*
 *  ucis_RemoveHistoryNode()
 *  This function removes the given history node and all its descendants.
 */
int 
ucis_RemoveHistoryNode(
    ucisT db,
    ucisHistoryNodeT historynode);

/*
 * UCIS_HISTORYNODE_TEST specialization
 *
 * ucisTestStatusT type is an enumerated type. When accessed directly as an attribute,
 * its attribute typing is UCIS_ATTR_INT and the returned value can be cast to 
 * the type defined below.
 */

typedef enum {
    UCIS_TESTSTATUS_OK,
    UCIS_TESTSTATUS_WARNING,    /* test warning ($warning called) */
    UCIS_TESTSTATUS_ERROR,      /* test error ($error called) */
    UCIS_TESTSTATUS_FATAL,      /* fatal test error ($fatal called) */
    UCIS_TESTSTATUS_MISSING,    /* test not run yet */
    UCIS_TESTSTATUS_MERGE_ERROR /* testdata record was merged with
                                     inconsistent data values */
} ucisTestStatusT;


typedef struct {
    ucisTestStatusT teststatus;
    double simtime;
    const char* timeunit;
    const char* runcwd;
    double cputime;
    const char* seed;
    const char* cmd;
    const char* args;
    int compulsory;
    const char* date;
    const char* username;
    double cost;
    const char* toolcategory;
} ucisTestDataT;

int 
ucis_SetTestData(
    ucisT db,
    ucisHistoryNodeT testhistorynode,
    ucisTestDataT* testdata);

int
ucis_GetTestData( ucisT db,
    ucisHistoryNodeT testhistorynode,
    ucisTestDataT* testdata);

ucisIteratorT
ucis_HistoryIterate (
    ucisT db,
    ucisHistoryNodeT historynode,
    ucisHistoryNodeKindT kind);

ucisHistoryNodeT
ucis_HistoryScan (
    ucisT db,
    ucisIteratorT iterator);

/*
 * Note that ucis_SetHistoryNodeParent() overwrites any existing setting
 */
int 
ucis_SetHistoryNodeParent(
    ucisT                 db,
    ucisHistoryNodeT      childnode,
    ucisHistoryNodeT      parentnode);

ucisHistoryNodeT
ucis_GetHistoryNodeParent(
    ucisT                     db,
    ucisHistoryNodeT          childnode);

/*
 *    ucis_GetHistoryKind (alias ucis_GetObjType)
 *
 *    This is a polymorphic function for acquiring an object type.
 *    This may return multiple bits set (specifically
 *    for history data objects). The return value MUST NOT be used as a mask.
 */
#define ucis_GetHistoryKind(db,obj) (ucisHistoryNodeKindT)ucis_GetObjType(db,obj)

/*------------------------------------------------------------------------------
 *  Scopes
 *
 *  The database is organized hierarchically, as per the design database: i.e.,
 *  there is a tree of module instances, each of a given module type.
 *
 *---------------------------------------------------------------------------*/

/* One-hot bits for ucisScopeTypeT: */
#define UCIS_TOGGLE          /* cover scope- toggle coverage scope: */ \
                             INT64_LITERAL(0x0000000000000001)
#define UCIS_BRANCH          /* cover scope- branch coverage scope: */ \
                             INT64_LITERAL(0x0000000000000002)
#define UCIS_EXPR            /* cover scope- expression coverage scope: */ \
                             INT64_LITERAL(0x0000000000000004)
#define UCIS_COND            /* cover scope- condition coverage scope: */ \
                             INT64_LITERAL(0x0000000000000008)
#define UCIS_INSTANCE        /* HDL scope- Design hierarchy instance: */ \
                             INT64_LITERAL(0x0000000000000010)
#define UCIS_PROCESS         /* HDL scope- process: */ \
                             INT64_LITERAL(0x0000000000000020)
#define UCIS_BLOCK           /* HDL scope- vhdl block, vlog begin-end: */ \
                             INT64_LITERAL(0x0000000000000040)
#define UCIS_FUNCTION        /* HDL scope- function: */ \
                             INT64_LITERAL(0x0000000000000080)
#define UCIS_FORKJOIN        /* HDL scope- Verilog fork-join block: */ \
                             INT64_LITERAL(0x0000000000000100)
#define UCIS_GENERATE        /* HDL scope- generate block: */ \
                             INT64_LITERAL(0x0000000000000200)
#define UCIS_GENERIC         /* cover scope- generic scope type: */ \
                             INT64_LITERAL(0x0000000000000400)
#define UCIS_CLASS           /* HDL scope- class type scope: */ \
                             INT64_LITERAL(0x0000000000000800)
#define UCIS_COVERGROUP      /* cover scope- covergroup type scope: */ \
                             INT64_LITERAL(0x0000000000001000)
#define UCIS_COVERINSTANCE   /* cover scope- covergroup instance scope: */ \
                             INT64_LITERAL(0x0000000000002000)
#define UCIS_COVERPOINT      /* cover scope- coverpoint scope: */ \
                             INT64_LITERAL(0x0000000000004000)
#define UCIS_CROSS           /* cover scope- cross scope: */ \
                             INT64_LITERAL(0x0000000000008000)
#define UCIS_COVER           /* cover scope- directive(SVA/PSL) cover: */ \
                             INT64_LITERAL(0x0000000000010000)
#define UCIS_ASSERT          /* cover scope- directive(SVA/PSL) assert: */ \
                             INT64_LITERAL(0x0000000000020000)
#define UCIS_PROGRAM         /* HDL scope- SV program instance: */ \
                             INT64_LITERAL(0x0000000000040000)
#define UCIS_PACKAGE         /* HDL scope- package instance: */ \
                             INT64_LITERAL(0x0000000000080000)
#define UCIS_TASK            /* HDL scope- task: */ \
                             INT64_LITERAL(0x0000000000100000)
#define UCIS_INTERFACE       /* HDL scope- SV interface instance: */ \
                             INT64_LITERAL(0x0000000000200000)
#define UCIS_FSM             /* cover scope- FSM coverage scope: */ \
                             INT64_LITERAL(0x0000000000400000)
#define UCIS_DU_MODULE       /* design unit- for instance type: */ \
                             INT64_LITERAL(0x0000000001000000)
#define UCIS_DU_ARCH         /* design unit- for instance type: */ \
                             INT64_LITERAL(0x0000000002000000)
#define UCIS_DU_PACKAGE      /* design unit- for instance type: */ \
                             INT64_LITERAL(0x0000000004000000)
#define UCIS_DU_PROGRAM      /* design unit- for instance type: */ \
                             INT64_LITERAL(0x0000000008000000)
#define UCIS_DU_INTERFACE    /* design unit- for instance type: */ \
                             INT64_LITERAL(0x0000000010000000)
#define UCIS_FSM_STATES      /* cover scope- FSM states coverage scope: */ \
                             INT64_LITERAL(0x0000000020000000)
#define UCIS_FSM_TRANS       /* cover scope- FSM transition coverage scope: */\
                             INT64_LITERAL(0x0000000040000000)
#define UCIS_COVBLOCK        /* cover scope- block coverage scope: */ \
                             INT64_LITERAL(0x0000000080000000)
#define UCIS_CVGBINSCOPE     /* cover scope- SV cover bin scope: */ \
                             INT64_LITERAL(0x0000000100000000)
#define UCIS_ILLEGALBINSCOPE /* cover scope- SV illegal bin scope: */ \
                             INT64_LITERAL(0x0000000200000000)
#define UCIS_IGNOREBINSCOPE  /* cover scope- SV ignore bin scope: */ \
                             INT64_LITERAL(0x0000000400000000)
#define UCIS_RESERVEDSCOPE   INT64_LITERAL(0xFF00000000000000)
#define UCIS_SCOPE_ERROR     /* error return code: */ \
                             INT64_LITERAL(0x0000000000000000)

#define UCIS_FSM_SCOPE      ((ucisScopeMaskTypeT)(UCIS_FSM |\
                                                  UCIS_FSM_STATES |\
                                                  UCIS_FSM_TRANS))

#define UCIS_CODE_COV_SCOPE ((ucisScopeMaskTypeT)(UCIS_BRANCH |\
                                                  UCIS_EXPR |\
                                                  UCIS_COND |\
                                                  UCIS_TOGGLE |\
                                                  UCIS_FSM_SCOPE |\
                                                  UCIS_BLOCK))

#define UCIS_DU_ANY ((ucisScopeMaskTypeT)(UCIS_DU_MODULE |\
                                          UCIS_DU_ARCH |\
                                          UCIS_DU_PACKAGE |\
                                          UCIS_DU_PROGRAM |\
                                          UCIS_DU_INTERFACE))

#define UCIS_CVG_SCOPE      ((ucisScopeMaskTypeT)(UCIS_COVERGROUP |\
                                                  UCIS_COVERINSTANCE |\
                                                  UCIS_COVERPOINT |\
                                                  UCIS_CVGBINSCOPE |\
                                                  UCIS_ILLEGALBINSCOPE |\
                                                  UCIS_IGNOREBINSCOPE |\
                                                  UCIS_CROSS))

#define UCIS_FUNC_COV_SCOPE ((ucisScopeMaskTypeT)(UCIS_CVG_SCOPE |\
                                                  UCIS_COVER))

#define UCIS_COV_SCOPE ((ucisScopeMaskTypeT)(UCIS_CODE_COV_SCOPE |\
                                             UCIS_FUNC_COV_SCOPE))

#define UCIS_VERIF_SCOPE ((ucisScopeMaskTypeT)(UCIS_COV_SCOPE |\
                                               UCIS_ASSERT |\
                                               UCIS_GENERIC))

#define UCIS_HDL_SUBSCOPE ((ucisScopeMaskTypeT)(UCIS_PROCESS |\
                                                UCIS_BLOCK |\
                                                UCIS_FUNCTION |\
                                                UCIS_FORKJOIN |\
                                                UCIS_GENERATE |\
                                                UCIS_TASK |\
                                                UCIS_CLASS))

#define UCIS_HDL_INST_SCOPE ((ucisScopeMaskTypeT)(UCIS_INSTANCE |\
                                                  UCIS_PROGRAM |\
                                                  UCIS_PACKAGE |\
                                                  UCIS_INTERFACE))

#define UCIS_HDL_DU_SCOPE ((ucisScopeMaskTypeT)(UCIS_DU_ANY))

#define UCIS_HDL_SCOPE ((ucisScopeMaskTypeT)(UCIS_HDL_SUBSCOPE |\
                                             UCIS_HDL_INST_SCOPE |\
                                             UCIS_HDL_DU_SCOPE))

#define UCIS_NO_SCOPES  ((ucisScopeMaskTypeT)INT64_ZERO)
#define UCIS_ALL_SCOPES ((ucisScopeMaskTypeT)INT64_NEG1)

typedef unsigned int ucisFlagsT;

/*
 *  Flags for scope data
 *  32-bits are allocated to flags.
 *  Flags are partitioned into four groups: GENERAL, TYPED, MARK and USER
 *  The general flags apply to all scopes
 *  The type-qualified flags have a meaning that can only be understood
 *  with respect to the scope type
 *  A single mark flag is reserved for temporary (volatile) use only
 *  The user flags are reserved for user applications
 */

#define UCIS_SCOPEMASK_GENERAL     0x0000FFFF;  /* 16 flags for general use   */
#define UCIS_SCOPEMASK_TYPED       0x07FF0000;  /* 11 flags for typed use     */
#define UCIS_SCOPEMASK_MARK        0x08000000;  /* 1 flag for mark (temporary) use */
#define UCIS_SCOPEMASK_USER        0xF0000000;  /* 4 flags for user extension */

/* General flags 0x0000FFFF (apply to all scope types) */
#define UCIS_INST_ONCE             0x00000001   /* An instance is instantiated only
                                                   once; code coverage is stored only in
                                                   the instance */
#define UCIS_ENABLED_STMT          0x00000002   /* statement coverage collected for scope */
#define UCIS_ENABLED_BRANCH        0x00000004   /* branch coverage coverage collected for scope */
#define UCIS_ENABLED_COND          0x00000008   /* condition coverage coverage collected for scope */
#define UCIS_ENABLED_EXPR          0x00000010   /* expression coverage coverage collected for scope */
#define UCIS_ENABLED_FSM           0x00000020   /* FSM coverage coverage collected for scope */
#define UCIS_ENABLED_TOGGLE        0x00000040   /* toggle coverage coverage collected for scope */

#define UCIS_SCOPE_UNDER_DU        0x00000100   /* is scope under a design unit? */
#define UCIS_SCOPE_EXCLUDED        0x00000200
#define UCIS_SCOPE_PRAGMA_EXCLUDED 0x00000400
#define UCIS_SCOPE_PRAGMA_CLEARED  0x00000800
#define UCIS_SCOPE_SPECIALIZED     0x00001000   /* Specialized scope may have data restrictions */
#define UCIS_UOR_SAFE_SCOPE        0x00002000   /* Scope construction is UOR compliant */
#define UCIS_UOR_SAFE_SCOPE_ALLCOVERS  0x00004000   /* Scope child coveritems are all UOR compliant */

/* Type-qualified flags x07FF0000 - flag locations may be reused for non-intersecting type sets */
#define UCIS_IS_TOP_NODE           0x00010000   /* UCIS_TOGGLE for top level toggle node */
#define UCIS_IS_IMMEDIATE_ASSERT   0x00010000   /* UCIS_ASSERTION for SV immediate assertions */
#define UCIS_SCOPE_CVG_AUTO        0x00010000   /* UCIS_COVERPOINT, UCIS_CROSS auto bin */
#define UCIS_SCOPE_CVG_SCALAR      0x00020000   /* UCIS_COVERPOINT, UCIS_CROSS SV scalar bin */
#define UCIS_SCOPE_CVG_VECTOR      0x00040000   /* UCIS_COVERPOINT, UCIS_CROSS SV vector bin */
#define UCIS_SCOPE_CVG_TRANSITION  0x00080000   /* UCIS_COVERPOINT, UCIS_CROSS transition bin */

#define UCIS_SCOPE_IFF_EXISTS      0x00100000   /* UCIS_COVERGROUP, UCIS_COVERINSTANCE has 'iff' clause */
#define UCIS_SCOPE_SAMPLE_TRUE     0x00200000   /* UCIS_COVERGROUP, UCIS_COVERINSTANCE has runtime samples */


#define UCIS_ENABLED_BLOCK         0x00800000   /* Block coverage collected for scope */
#define UCIS_SCOPE_BLOCK_ISBRANCH  0x01000000   /* UCIS_BBLOCK scope contributes to both Block and Branch coverage */


#define UCIS_SCOPE_EXPR_ISHIERARCHICAL 0x02000000  /* UCIS_EXPR, UCIS_COND represented hierarchically */

/* The temporary mark flag */
#define UCIS_SCOPEFLAG_MARK        0x08000000   /* flag for temporary mark */

/* The reserved flags */
#define UCIS_SCOPE_INTERNAL        0xF0000000

/*
 *  ucisSourceT
 *  Enumerated type to encode the source type of a scope, if needed.  Note that
 *  scope type can have an effect on how the system regards escaped identifiers
 *  within design hierarchy.
 */
typedef enum {
    UCIS_VHDL,
    UCIS_VLOG,          /* Verilog */
    UCIS_SV,            /* SystemVerilog */
    UCIS_SYSTEMC,
    UCIS_PSL_VHDL,      /* assert/cover in PSL VHDL */
    UCIS_PSL_VLOG,      /* assert/cover in PSL Verilog */
    UCIS_PSL_SV,        /* assert/cover in PSL SystemVerilog */
    UCIS_PSL_SYSTEMC,   /* assert/cover in PSL SystemC */
    UCIS_E,
    UCIS_VERA,
    UCIS_NONE,          /* if not important */
    UCIS_OTHER,         /* to refer to user-defined attributes */
    UCIS_SOURCE_ERROR   /* for error cases */
}   ucisSourceT;

/*
 *  ucis_CreateScope()
 *  Create the given scope underneath the given parent scope; if parent is
 *  NULL, create as root scope.
 *  Returns scope pointer or NULL if error.
 *  Note: use ucis_CreateInstance() for UCIS_INSTANCE scopes.
 *  Note: in write streaming mode, "name" is not copied; it needs to be
 *  preserved unchanged until the next ucis_WriteStream* call or the next
 *  ucis_Create* call.
 */
ucisScopeT
ucis_CreateScope(
    ucisT               db,
    ucisScopeT          parent,
    const char*         name,
    ucisSourceInfoT*    srcinfo,
    int                 weight,         /* negative for none */
    ucisSourceT         source,
    ucisScopeTypeT      type,           /* type of scope to create */
    ucisFlagsT          flags);
/*
 * Specialized constructors
 * 
 * Specialized constructors are used to ensure data coherence by 
 * enforcing the collection of necessary data in an atomic operation,
 * at construction time.
 *
 * Specialized constructors add constraints to database object *data*
 * but still operate within the database *schema*.
 *
 * The data consistency enforced by the specialization may be a
 * pre-requisite to certain optimizations, and these optimizations may be
 * vendor-specific.
 *
 * For example, a specialized scope may have a forced bin collection shape
 * and default attributes populated from a default list of values. 
 * It may also require that certain attributes be present.
 * The intent is to support common data constructs that depend on 
 * certain assumptions.
 *
 * Specialized constructors may enforce attribute and bin shape data, but
 * they do not imply the presence of data that is not accessible to 
 * the API routines
 *
 * The UCIS_SCOPE_SPECIALIZED is set on a scope if a specialized constructor
 * was used.
 *
 * The specialized scope constructors are:
 *
 * ucis_CreateInstance() - enforce the rule that an instance has a DU
 * ucis_CreateInstanceByName() - enforce the rule that an instance has a DU
 * ucis_CreateCross()    - enforce the cross relationship to coverpoints
 * ucis_CreateCrossByName() - enforce the cross relationship to coverpoints
 * ucis_CreateToggle()   - add properties to identify the basic toggle metrics
 */

/*
 * ucis_CreateInstance() specialized constructor
 *
 * This constructor enforces the addition of the DU relationship to an
 * instance scope
 * It is used to construct scopes of types: 
 * - UCIS_INSTANCE which must have a du_scope of one of the UCIS_DU_* types
 * - UCIS_COVERINSTANCE, which must have a du_scope of UCIS_COVERGROUP type
 */

ucisScopeT
ucis_CreateInstance(
    ucisT               db,
    ucisScopeT          parent,
    const char*         name,
    ucisSourceInfoT*    fileinfo,
    int                 weight,         /* negative for none */
    ucisSourceT         source,
    ucisScopeTypeT      type,           /* UCIS_INSTANCE, UCIS_COVERINSTANCE
                                           type of scope to create */
    ucisScopeT          du_scope,       /* if type==UCIS_INSTANCE, this is a
                                           scope of type UCIS_DU_*; if
                                           type==UCIS_COVERINSTANCE, this is a
                                           scope of type UCIS_COVERGROUP */
    ucisFlagsT          flags);

/*
 * ucis_CreateInstanceByName() specialized constructor
 *
 * This is the write-streaming version of ucis_CreateInstance() where the 
 * design unit or covergroup is identified by name (because in-memory pointer
 * identification is not possible). Note that this routine marshalls data for
 * write-streaming but it is not flushed until a ucis_WriteStream* or ucis_Create* 
 * call is made
 * The strings pointed to by "name" and "du_name" must be retained
 * until then.
 *
 * When called in write-streaming mode, the parent is ignored, however the routine
 * may also be used in in-memory mode. In this case, the du_name is converted
 * to a ucisScopeT and the routine behaves as ucis_CreateInstance().
 */

ucisScopeT
ucis_CreateInstanceByName(
    ucisT               db,
    ucisScopeT          parent,
    const char*         name,
    ucisSourceInfoT*    fileinfo,
    int                 weight,
    ucisSourceT         source,
    ucisScopeTypeT      type,          /* UCIS_INSTANCE, UCIS_COVERINSTANCE
                                          type of scope to create */
    char*               du_name,       /* name for instance's design unit, or
                                          coverinstance's covergroup type */
    int                 flags);

/*
 * ucis_CreateCross() specialized constructor
 *
 * This constructor uses the data from existing parent covergroup or
 * coverinstance to infer the basic cross scope construction
 * The parental coverpoints must exist.
 */

ucisScopeT
ucis_CreateCross(
    ucisT               db,
    ucisScopeT          parent,         /* covergroup or cover instance */
    const char*         name,
    ucisSourceInfoT*    fileinfo,
    int                 weight,
    ucisSourceT         source,
    int                 num_points,     /* number of crossed coverpoints */
    ucisScopeT*         points);        /* array of coverpoint scopes */

/*
 * ucis_CreateCrossByName() specialized constructor
 *
 * This is the write-streaming version of ucis_CreateCross() where the 
 * crossed coverpoints are identified by name (because in-memory pointer
 * identification is not possible)
 * Note that this routine marshalls data for
 * write-streaming but it is not flushed until a ucis_WriteStream* or ucis_Create* 
 * call is made
 * The strings pointed to by "name" and "du_name" must be retained
 * until then.
 * 
 */

ucisScopeT
ucis_CreateCrossByName(
    ucisT               db,
    ucisScopeT          parent,         /* covergroup or cover instance */
    const char*         name,
    ucisSourceInfoT*    fileinfo,
    int                 weight,
    ucisSourceT         source,
    int                 num_points,     /* number of crossed coverpoints */
    char**              point_names);   /* array of coverpoint names */

/*
 * ucis_CreateToggle() specialized constructor
 * 
 * This constructor enforces type-dependent bin shapes and associated attributes
 * onto a toggle scope.
 *
 * The bins under this scope must be created after the scope has been created.
 * 
 * The canonical_name attribute is used to associate the toggle name, which
 * is a local name, with a global net name (similar to the Verilog simnet concept)
 * The canonical_name is recovered with ucis_GetStringProperty() for the
 * UCIS_STR_TOGGLE_CANON_NAME property
 *
 * The toggle_metric value is used to identify the specific shape specialization
 * The toggle_metric is recovered with ucis_GetIntProperty() for the
 * UCIS_INT_TOGGLE_METRIC property
 *
 * The toggle_type value is used to identify whether the toggling object is a
 * net or register.
 * The toggle_type is recovered with ucis_GetIntProperty() for the
 * UCIS_INT_TOGGLE_TYPE property
 *
 * The toggle_dir value is used to identify the toggle direction for a port toggle.
 * The toggle_dir is recovered with ucis_GetIntProperty() for the
 * UCIS_INT_TOGGLE_DIR property
 */

typedef enum {                        /* Implicit toggle local bins metric is:  */
    UCIS_TOGGLE_METRIC_NOBINS = 1,    /* Toggle scope has no local bins         */
    UCIS_TOGGLE_METRIC_ENUM,          /* UCIS:ENUM                              */
    UCIS_TOGGLE_METRIC_TRANSITION,    /* UCIS:TRANSITION                        */
    UCIS_TOGGLE_METRIC_2STOGGLE,      /* UCIS:2STOGGLE                          */
    UCIS_TOGGLE_METRIC_ZTOGGLE,       /* UCIS:ZTOGGLE                           */
    UCIS_TOGGLE_METRIC_XTOGGLE        /* UCIS:XTOGGLE                           */
} ucisToggleMetricT;

typedef enum {
    UCIS_TOGGLE_TYPE_NET = 1,
    UCIS_TOGGLE_TYPE_REG = 2
} ucisToggleTypeT;

typedef enum {
    UCIS_TOGGLE_DIR_INTERNAL = 1,   /* non-port: internal wire or variable */
    UCIS_TOGGLE_DIR_IN,             /* input port */
    UCIS_TOGGLE_DIR_OUT,            /* output port */
    UCIS_TOGGLE_DIR_INOUT           /* inout port */
} ucisToggleDirT;

ucisScopeT
ucis_CreateToggle(
    ucisT               db,
    ucisScopeT          parent,         /* toggle will be created in this scope */
    const char*         name,           /* name of toggle object                */
    const char*         canonical_name, /* canonical_name of toggle object      */
    ucisFlagsT          flags,
    ucisToggleMetricT   toggle_metric,
    ucisToggleTypeT     toggle_type,
    ucisToggleDirT      toggle_dir);

/*
 *  Utilities for parsing and composing design unit scope names.  These are of
 *  the form "library.primary(secondary)#instance_num"
 *
 *  Note: these utilities each employ a static dynamic string (one for the
 *  "Compose" function, one for the "Parse" function).  That means that values
 *  are only valid until the next call to the respective function; if you need
 *  to hold the memory across separate calls you must copy it.
 */
const char*                             /* Return value is in temp storage */
ucis_ComposeDUName(
    const char*         library_name,  /* input names */
    const char*         primary_name,
    const char*         secondary_name);

void
ucis_ParseDUName(
    const char*         du_name,       /* input to function */
    const char**        library_name,  /* output names */
    const char**        primary_name,
    const char**        secondary_name);

int
ucis_RemoveScope(
    ucisT               db,     /* database context */
    ucisScopeT          scope); /* scope to remove */

/*
 *  ucis_GetScopeType()
 *  Get type of scope.
 *  Returns UCIS_SCOPE_ERROR if error.
 */
ucisScopeTypeT
ucis_GetScopeType(
    ucisT               db,
    ucisScopeT          scope);

/*
 *  ucis_GetObjType (alias ucis_GetHistoryKind)
 *
 *  Returns UCIS_SCOPE_ERROR if error.
 *  Returns UCIS_HISTORYNODE_TEST when obj is a test data record.
 *  Returns UCIS_HISTORYNODE_MERGE when obj is a merge record.
 *  Otherwise, returns the scope type ucisScopeTypeT:
 *      [See ucisScopeTypeT ucis_GetScopeType(ucisT db, ucisScopeT scope)]
 *
 *  This is a polymorphic function for acquiring an object type.
 *  History node types, unlike scope and coveritem types, are not one-hot,
 *  and should therefore not be used as a mask
 */
ucisObjTypeT
ucis_GetObjType(
    ucisT           db,
    ucisObjT        obj);

/*
 *  ucis_GetScopeFlags()
 *  Get scope flags.
 */
ucisFlagsT
ucis_GetScopeFlags(
    ucisT               db,
    ucisScopeT          scope);

/*
 *  ucis_SetScopeFlags()
 *  Set scope flags.
 */
void
ucis_SetScopeFlags(
    ucisT               db,
    ucisScopeT          scope,
    ucisFlagsT          flags);

/*
 *  ucis_GetScopeFlag()
 *  Fancier interface for getting a single flag bit.
 *  Return 1 if specified scope's flag bit matches the given mask.
 */
int
ucis_GetScopeFlag(
    ucisT               db,
    ucisScopeT          scope,
    ucisFlagsT          mask);

/*
 *  ucis_SetScopeFlag()
 *  Set bits in the scope's flag field with respect to the given mask --
 *  set all bits to 0 or 1.
 */
void
ucis_SetScopeFlag(
    ucisT               db,
    ucisScopeT          scope,
    ucisFlagsT          mask,
    int                 bitvalue);  /* 0 or 1 only */

/*
 *  ucis_GetScopeSourceInfo()
 *  Gets source information (file/line/token) for the given scope.
 *  Note: does not apply to toggle nodes.
 */
int
ucis_GetScopeSourceInfo(
    ucisT               db,
    ucisScopeT          scope,
    ucisSourceInfoT*    sourceinfo);

/*
 *  ucis_SetScopeSourceInfo()
 *  Sets source information (file/line/token) for the given scope.
 *  Returns 0 on success, non-zero on failure.
 */
int
ucis_SetScopeSourceInfo(
    ucisT               db,
    ucisScopeT          scope,
    ucisSourceInfoT*    sourceinfo);

/*
 *  ucis_GetIthCrossedCvp()
 *  Get crossed coverpoint of scope specified by the index, if scope is a
 *  cross scope.
 *  Returns 0 if success, non-zero if error.
 */
int
ucis_GetIthCrossedCvp(
    ucisT               db,
    ucisScopeT          scope,
    int                 index,
    ucisScopeT*         point_scope);

/*
 *  ucis_MatchDU()
 *  Given a design unit name, get the design unit scope in the database.
 *  Returns NULL if no match is found.
 */
ucisScopeT
ucis_MatchDU(
    ucisT               db,
    const char*         name);


/*----------------------------------------------------------------------------
 *  Creating coveritems (bins, with an associated count.)
 *--------------------------------------------------------------------------*/

/* One-hot bits for ucisCoverTypeT: */
#define UCIS_CVGBIN          /* For SV Covergroups: */ \
                             INT64_LITERAL(0x0000000000000001)
#define UCIS_COVERBIN        /* For cover directives- pass: */ \
                             INT64_LITERAL(0x0000000000000002)
#define UCIS_ASSERTBIN       /* For assert directives- fail: */ \
                             INT64_LITERAL(0x0000000000000004)
#define UCIS_STMTBIN         /* For Code coverage(Statement): */ \
                             INT64_LITERAL(0x0000000000000020)
#define UCIS_BRANCHBIN      /* For Code coverage(Branch): */ \
                             INT64_LITERAL(0x0000000000000040)
#define UCIS_EXPRBIN         /* For Code coverage(Expression): */ \
                             INT64_LITERAL(0x0000000000000080)
#define UCIS_CONDBIN         /* For Code coverage(Condition): */ \
                             INT64_LITERAL(0x0000000000000100)
#define UCIS_TOGGLEBIN       /* For Code coverage(Toggle): */ \
                             INT64_LITERAL(0x0000000000000200)
#define UCIS_PASSBIN         /* For assert directives- pass count: */ \
                             INT64_LITERAL(0x0000000000000400)
#define UCIS_FSMBIN          /* For FSM coverage: */ \
                             INT64_LITERAL(0x0000000000000800)
#define UCIS_USERBIN         /* User-defined coverage: */ \
                             INT64_LITERAL(0x0000000000001000)
#define UCIS_GENERICBIN      UCIS_USERBIN
#define UCIS_COUNT           /* user-defined count, not in coverage: */ \
                             INT64_LITERAL(0x0000000000002000)
#define UCIS_FAILBIN         /* For cover directives- fail count: */ \
                             INT64_LITERAL(0x0000000000004000)
#define UCIS_VACUOUSBIN      /* For assert- vacuous pass count: */ \
                             INT64_LITERAL(0x0000000000008000)
#define UCIS_DISABLEDBIN     /* For assert- disabled count: */ \
                             INT64_LITERAL(0x0000000000010000)
#define UCIS_ATTEMPTBIN      /* For assert- attempt count: */ \
                             INT64_LITERAL(0x0000000000020000)
#define UCIS_ACTIVEBIN       /* For assert- active thread count: */ \
                             INT64_LITERAL(0x0000000000040000)
#define UCIS_IGNOREBIN       /* For SV Covergroups: */ \
                             INT64_LITERAL(0x0000000000080000)
#define UCIS_ILLEGALBIN      /* For SV Covergroups: */ \
                             INT64_LITERAL(0x0000000000100000)
#define UCIS_DEFAULTBIN      /* For SV Covergroups: */ \
                             INT64_LITERAL(0x0000000000200000)
#define UCIS_PEAKACTIVEBIN   /* For assert- peak active thread count: */ \
                             INT64_LITERAL(0x0000000000400000)
#define UCIS_BLOCKBIN        /* For Code coverage(Block): */ \
                             INT64_LITERAL(0x0000000001000000)
#define UCIS_USERBITS        /* For user-defined coverage: */ \
                             INT64_LITERAL(0x00000000FE000000)
#define UCIS_RESERVEDBIN     INT64_LITERAL(0xFF00000000000000)

/* Coverage item types */

#define UCIS_COVERGROUPBINS \
            ((ucisCoverMaskTypeT)(UCIS_CVGBIN | \
                                  UCIS_IGNOREBIN | \
                                  UCIS_ILLEGALBIN | \
                                  UCIS_DEFAULTBIN))

#define UCIS_FUNC_COV \
            ((ucisCoverMaskTypeT)(UCIS_COVERGROUPBINS | \
                                  UCIS_COVERBIN | \
                                  UCIS_SCBIN))

#define UCIS_CODE_COV \
            ((ucisCoverMaskTypeT)(UCIS_STMTBIN | \
                                  UCIS_BRANCHBIN | \
                                  UCIS_EXPRBIN | \
                                  UCIS_CONDBIN | \
                                  UCIS_TOGGLEBIN | \
                                  UCIS_FSMBIN |\
                                  UCIS_BLOCKBIN))

#define UCIS_ASSERTIONBINS \
            ((ucisCoverMaskTypeT)(UCIS_ASSERTBIN |\
                                  UCIS_PASSBIN |\
                                  UCIS_VACUOUSBIN |\
                                  UCIS_DISABLEDBIN |\
                                  UCIS_ATTEMPTBIN |\
                                  UCIS_ACTIVEBIN |\
                                  UCIS_PEAKACTIVEBIN))

#define UCIS_COVERDIRECTIVEBINS \
            ((ucisCoverMaskTypeT)(UCIS_COVERBIN |\
                                  UCIS_FAILBIN))
#define UCIS_NO_BINS  ((ucisCoverMaskTypeT)INT64_ZERO)
#define UCIS_ALL_BINS ((ucisCoverMaskTypeT)INT64_NEG1)

/*------------------------------------------------------------------------------
 *  Creating and manipulating coveritems
 *----------------------------------------------------------------------------*/

/*
 *  Flags for coveritem data
 *  32-bits are allocated to flags.
 *  Flags are partitioned into four groups: GENERAL, TYPED, MARK and USER
 *  The general flags apply to all scopes
 *  The type-determined flags have a meaning that can only be understood
 *  with respect to the scope type
 *  The user flags are reserved for user applications
 */

#define UCIS_COVERITEMMASK_GENERAL     0x0000FFFF;  /* 16 flags for general use   */
#define UCIS_COVERITEMMASK_TYPED       0x07FF0000;  /* 11 flags for typed use     */
#define UCIS_COVERITEMMASK_MARK        0x08000000;  /* 1 flag for mark (temporary) use */
#define UCIS_COVERITEMMASK_USER        0xF0000000;  /* 4 flags for user extension */

/* General flags 0x0000FFFF (apply to all coveritem types */
#define UCIS_IS_32BIT        0x00000001  /* data is 32 bits */
#define UCIS_IS_64BIT        0x00000002  /* data is 64 bits */
#define UCIS_IS_VECTOR       0x00000004  /* data is a vector */
#define UCIS_HAS_GOAL        0x00000008  /* goal included */
#define UCIS_HAS_WEIGHT      0x00000010  /* weight included */

#define UCIS_EXCLUDE_PRAGMA  0x00000020  /* excluded by pragma  */
#define UCIS_EXCLUDE_FILE    0x00000040  /* excluded by file; does not
                                            count in total coverage */
#define UCIS_EXCLUDE_INST    0x00000080  /* for instance-specific exclusions */
#define UCIS_EXCLUDE_AUTO    0x00000100  /* for automatic exclusions */

#define UCIS_ENABLED         0x00000200  /* generic enabled flag; if disabled,
                                            still counts in total coverage */
#define UCIS_HAS_LIMIT       0x00000400  /* for limiting counts */
#define UCIS_HAS_COUNT       0x00000800  /* has count in ucisCoverDataValueT? */
#define UCIS_IS_COVERED      0x00001000  /* set if object is covered */
#define UCIS_UOR_SAFE_COVERITEM        0x00002000   /* Coveritem construction is UOR compliant */
#define UCIS_CLEAR_PRAGMA    0x00004000

/* Type-qualified flags 0x07FF0000 - flag locations may be reused for non-intersecting type sets */
#define UCIS_HAS_ACTION      0x00010000  /* UCIS_ASSERTBIN */
#define UCIS_IS_TLW_ENABLED  0x00020000  /* UCIS_ASSERTBIN */
#define UCIS_LOG_ON          0x00040000  /* UCIS_COVERBIN, UCIS_ASSERTBIN */
#define UCIS_IS_EOS_NOTE     0x00080000  /* UCIS_COVERBIN, UCIS_ASSERTBIN */

#define UCIS_IS_FSM_RESET    0x00010000  /* UCIS_FSMBIN */
#define UCIS_IS_FSM_TRAN     0x00020000  /* UCIS_FSMBIN */
#define UCIS_IS_BR_ELSE      0x00010000  /* UCIS_BRANCHBIN */

#define UCIS_BIN_IFF_EXISTS  0x00010000  /* UCIS_CVGBIN UCIS_IGNOREBIN UCIS_ILLEGALBIN UCIS_DEFAULTBIN */
#define UCIS_BIN_SAMPLE_TRUE 0x00020000  /* UCIS_CVGBIN UCIS_IGNOREBIN UCIS_ILLEGALBIN UCIS_DEFAULTBIN */

#define UCIS_IS_CROSSAUTO    0x00040000  /* UCIS_CROSS */

/* The temporary mark flag */
#define UCIS_COVERFLAG_MARK  0x08000000  /* flag for temporary mark */

/* The reserved user flags */
#define UCIS_USERFLAGS       0xF0000000  /* reserved for user flags */

#define UCIS_FLAG_MASK       0xFFFFFFFF

#define UCIS_EXCLUDED        (UCIS_EXCLUDE_FILE | UCIS_EXCLUDE_PRAGMA | UCIS_EXCLUDE_INST | UCIS_EXCLUDE_AUTO)

/*
 *  Type representing coveritem data.
 */
typedef union {
    uint64_t            int64;      /* if UCIS_IS_64BIT */
    uint32_t            int32;      /* if UCIS_IS_32BIT */
    unsigned char*      bytevector; /* if UCIS_IS_VECTOR */
} ucisCoverDataValueT;

typedef struct {
    ucisCoverTypeT      type;       /* type of coveritem */
    ucisFlagsT          flags;      /* as above, validity of fields below */
    ucisCoverDataValueT data;
    /*
     *  This "goal" value is used to determine whether an individual bin is
     *  covered; it corresponds to "at_least" in a covergroup:
     */
    int                 goal;       /* if UCIS_HAS_GOAL */
    int                 weight;     /* if UCIS_HAS_WEIGHT */
    int                 limit;      /* if UCIS_HAS_LIMIT */
    int                 bitlen;     /* length of data.bytevector in bits,
                                     * extra bits are lower order bits of the
                                     * last byte in the byte vector
                                     */
} ucisCoverDataT;

/*
 *  ucis_CreateNextCover()
 *  Create the next coveritem in the given scope.
 *  Returns the index number of the created coveritem, -1 if error.
 *  Note: in write streaming mode, "name" is not copied; it needs to be
 *  preserved unchanged until the next ucis_WriteStream* call or the next
 *  ucis_Create* call.
 */
int
ucis_CreateNextCover(
    ucisT               db,
    ucisScopeT          parent,     /* coveritem will be created in this scope */
    const char*         name,       /* name of coveritem, can be NULL */
    ucisCoverDataT*     data,       /* associated data for coverage */
    ucisSourceInfoT*    sourceinfo);

/*
 *  ucis_RemoveCover()
 *  This function removes the given cover from its parent.
 *  There is no effect of this function in streaming modes.
 *  A successful operation will return 0, and -1 for error.
 *  Note: Coveritems can not be removed from scopes of type UCIS_ASSERT,
 *  need to remove the whole scope instead.
 *  Similarly, coveritems from scopes of type UCIS_TOGGLE which has toggle kind
 *  UCIS_TOGGLE_SCALAR, or UCIS_TOGGLE_SCALAR_EXT, or UCIS_TOGGLE_REG_SCALAR,
 *  or UCIS_TOGGLE_REG_SCALAR_EXT cannot be removed. The scope needs to be
 *  removed in this case too.
 *  Also, this function should be used carefully if it is
 *  used during iteration of coveritems using any coveritem
 *  iteration API, otherwise the iteration may not be complete.
 */
int
ucis_RemoveCover(
    ucisT               db,
    ucisScopeT          parent,
    int                 coverindex);

/*
 *  ucis_GetCoverFlags()
 *  Get coveritem's flag.
 */
ucisFlagsT
ucis_GetCoverFlags(
    ucisT               db,
    ucisScopeT          parent,      /* parent scope of coveritem */
    int                 coverindex); /* index of coveritem in parent */

/*
 *  ucis_GetCoverFlag()
 *  Return 1 if specified coveritem's flag bit matches the given mask.
 */
int
ucis_GetCoverFlag(
    ucisT               db,
    ucisScopeT          parent,     /* parent scope of coveritem */
    int                 coverindex, /* index of coveritem in parent */
    ucisFlagsT          mask);

/*
 *  ucis_SetCoverFlag()
 *  Set bits in the coveritem's flag field with respect to the given mask --
 *  set all bits to 0 or 1.
 */
void
ucis_SetCoverFlag(
    ucisT               db,
    ucisScopeT          parent,     /* parent scope of coveritem */
    int                 coverindex, /* index of coveritem in parent */
    ucisFlagsT          mask,
    int                 bitvalue);  /* 0 or 1 only */

/*
 *  ucis_GetCoverData()
 *  Get all the data for a coverage item, returns 0 for success, and non-zero
 *  for any error.  It is the user's responsibility to save the returned data,
 *  the next call to this function will invalidate the returned data.
 *  Note: any of the data arguments may be NULL, in which case data is not
 *  retrieved.
 */
int
ucis_GetCoverData(
    ucisT               db,
    ucisScopeT          parent,     /* parent scope of coveritem */
    int                 coverindex, /* index of coveritem in parent */
    char**              name,
    ucisCoverDataT*     data,
    ucisSourceInfoT*    sourceinfo);

/*
 *  ucis_SetCoverData()
 *  Set the data for a coverage item, returns 0 for success, and non-zero
 *  for any error.  It is the user's responsibility to make all the data
 *  fields valid.
 */
int
ucis_SetCoverData(
    ucisT               db,
    ucisScopeT          parent,     /* parent scope of coveritem */
    int                 coverindex, /* index of coveritem in parent */
    ucisCoverDataT*     data);

/*
 *  ucis_IncrementCover()
 *  Increment the data count for a coverage data item, if not a vector item.
 */
int
ucis_IncrementCover(
    ucisT               db,
    ucisScopeT          parent,     /* parent scope of coveritem */
    int                 coverindex, /* index of coveritem in parent */
    int64_t             increment); /* added to current count */

/*------------------------------------------------------------------------------
 * Test traceability support
 * Test traceability is based on a data handle type that encapsulates a list  
 * of history node records in an opaque handle type - ucisHistoryNodeListT
 *
 * The history node list is a data type in its own right but lists are not stored
 * in the database as standalone objects, only in association with coveritems.
 * 
 * A history node list may be iterated with ucis_HistoryNodeListIterate
 * and ucis_HistoryScan.
 * Note though that the iterator object is distinct from the list.
 * List and iterator memory management must both be individually considered 
 * when using history node list iterators.
 *
 * A list of history nodes may be associated with any number of coveritems in
 * the hierarchy, for example to express the relationship between tests and
 * the target coveritem(s) that were incremented when the test was run.
 *
 * If the application constructs a list, and associates the list with one or
 * more coveritems, the original list must be freed when the associations have
 * been made. 
 *
 * Each association between a list and a coveritem may be labeled with an
 * integral 'association type' to indicate the semantic of the association.
 * For example the UCIS_ASSOC_TESTHIT semantic
 * is pre-defined to represent the association between a list of 
 * tests and the coveritems that were incremented by the test
 *
 *----------------------------------------------------------------------------*/

#define UCIS_ASSOC_TESTHIT                0
#define UCIS_ASSOC_FORMALLY_UNREACHABLE   1

typedef void * ucisHistoryNodeListT; /* conceptually a list of history nodes */
typedef int    ucisAssociationTypeT; /* see UCIS_ASSOC #defines above */

/*
 * History node list management
 * - list construction
 * - add and delete history nodes
 * - list deletion (to free memory associated with list, not the history nodes)
 * - list query (by iteration).
 * - list association to coveritem(s)
 *
 * The model for this functionality is that history node lists are 
 * data-marshalling constructs created by the application, or returned
 * by ucis_GetHistoryNodeListAssoc()
 *
 * The list itself is required to be non-duplicative and non-ordered
 * If a list-element is re-added, it is not an error, although nothing is added
 * The list key (to determine duplication) is the ucisHistoryNodeT handle.
 * If a non-existent element is removed, it is not an error, nothing is removed
 * No list ordering is guaranteed or implied. Applications must not rely
 * on list order.
 *
 * An existing list may be associated with one or more coveritems
 * (one association is made per call to ucis_SetHistoryNodeListAssoc()
 * but the call may be repeated on multiple coveritems using the same list)
 *
 * Events that change or invalidate either the history nodes on the list or  
 * the coveritems, may also invalidate the lists and the iterations on
 * them.
 *
 * The memory model for this process is that the call to 
 * ucis_SetHistoryNodeListAssoc() causes the replacement of any pre-existing
 * association.
 * ucis_SetHistoryNodeListAssoc() causes database *copies* of the list data
 * to be constructed.
 * A call to ucis_GetHistoryNodeListAssoc() returns a read-only list, valid
 * until another call to ucis_GetHistoryNodeListAssoc() or another
 * invalidating event.
 * An empty list and a NULL pointer are both representations of zero
 * history nodes associated with a coveritem. These are equivalent when
 * presented to ucis_SetHistoryNodeListAssoc() (both will delete any existing
 * association), but ucis_GetHistoryNodeListAssoc() returns a NULL list pointer
 * in both cases. That is, the database does not distinguish between an
 * empty list and the absence of a list.
 * 
 * Consequences of these design assumptions are:
 * - the application copy of a list created by ucis_CreateHistoryNodeList() and
 *   used in a ucis_SetHistoryNodeListAssoc() call can (though need not)  
 *   immediately be freed if desired
 * - Updating an existing list associated with a coveritem is a 
 *   read-duplicate-modify-replace operation (the list associated with a coveritem
 *   cannot be directly edited)
 * - Either an empty list or a NULL list pointer deletes the existing association
 */

ucisHistoryNodeListT
ucis_CreateHistoryNodeList(ucisT db);

/*
 * ucis_FreeHistoryNodeList frees only the memory associated with the list,
 * it does not affect the history nodes on the list.
 *
 * This routine must only be used on a list created by the user with 
 * ucis_CreateHistoryNodeList(). Lists returned from the kernel should
 * not be freed. Generally, lists returned from the kernel are valid only
 * until the next list-management call, or the database is closed, whichever
 * is first. Lists are also invalidated by coveritem removal events.
 * Any iterators on a list that is invalidated by one of these
 * events, also immediately become invalid.
 */
int
ucis_FreeHistoryNodeList(ucisT db, 
                         ucisHistoryNodeListT historynode_list);

int
ucis_AddToHistoryNodeList(ucisT db, 
                          ucisHistoryNodeListT historynode_list, 
                          ucisHistoryNodeT historynode);

int
ucis_RemoveFromHistoryNodeList(ucisT db, 
                               ucisHistoryNodeListT historynode_list, 
                               ucisHistoryNodeT historynode);

ucisIteratorT
ucis_HistoryNodeListIterate(ucisT db,
                            ucisHistoryNodeListT historynode_list);


/*
 * Association of history node list with coveritem - set and query
 */

int
ucis_SetHistoryNodeListAssoc(ucisT db,
                    ucisScopeT scope, 
                    int coverindex,
                    ucisHistoryNodeListT historynode_list,
                    ucisAssociationTypeT assoc_type);

ucisHistoryNodeListT
ucis_GetHistoryNodeListAssoc(ucisT db,
                    ucisScopeT scope, 
                    int coverindex,
                    ucisAssociationTypeT assoc_type);


/*------------------------------------------------------------------------------
 *  Summary coverage statistics.
 *
 *  This interface allows quick access to aggregated coverage and statistics
 *  for different kinds of coverage, and some overall statistics for the
 *  database.
 *----------------------------------------------------------------------------*/

#define UCIS_CVG_INST		0x00000001 /* same as $get_coverage in SystemVerilog */
#define UCIS_CVG_DU		    0x00000002 /* Covergroup coverage, per design unit */
#define UCIS_BLOCK_INST		0x00000004 /* Block coverage, per design instance */
#define UCIS_BLOCK_DU		0x00000008 /* Block coverage, per design unit */
#define UCIS_STMT_INST		0x00000010 /* Statement coverage, per design instance */
#define UCIS_STMT_DU		0x00000020 /* Statement coverage, per design unit */
#define UCIS_BRANCH_INST	0x00000040 /* Branch coverage, per design instance */
#define UCIS_BRANCH_DU		0x00000080 /* Branch coverage, per design unit */
#define UCIS_EXPR_INST		0x00000100 /* Expression coverage, per design instance */
#define UCIS_EXPR_DU		0x00000200 /* Expression coverage, per design unit */
#define UCIS_COND_INST		0x00000400 /* Condition coverage, per design instance */
#define UCIS_COND_DU		0x00000800 /* Condition coverage, per design unit */
#define UCIS_TOGGLE_INST	0x00001000 /* Toggle coverage, per design instance */
#define UCIS_TOGGLE_DU		0x00002000 /* Toggle coverage, per design unit */
#define UCIS_FSM_ST_INST	0x00004000 /* FSM state coverage, per design instance */
#define UCIS_FSM_ST_DU		0x00008000 /* FSM state coverage, per design unit */
#define UCIS_FSM_TR_INST	0x00010000 /* FSM transition coverage, per design instance */
#define UCIS_FSM_TR_DU		0x00020000 /* FSM transition coverage, per design unit */
#define UCIS_USER_INST		0x00040000 /* User-defined coverage, per design instance */
#define UCIS_USER_DU		0x00080000 /* User-defined coverage, per design unit */
#define UCIS_ASSERT_PASS_INST	0x00100000 /* Assertion directive passes, per design instance */
#define UCIS_ASSERT_FAIL_INST	0x00200000 /* Assertion directive failures, per design instance */
#define UCIS_ASSERT_VPASS_INST		0x00400000 /* Assertion directive vacuous passes, per design instance */
#define UCIS_ASSERT_DISABLED_INST	0x00800000 /* Assertion directive disabled,per design instance */
#define UCIS_ASSERT_ATTEMPTED_INST	0x01000000 /* Assertion directive attempted, per design instance */
#define UCIS_ASSERT_ACTIVE_INST		0x02000000 /* Assertion directive active, per design instance */
#define UCIS_ASSERT_PACTIVE_INST	0x04000000 /* Assertion directive peakactive, per design instance */
#define UCIS_COVER_INST		0x08000000 /* Cover directive, per design instance*/
#define UCIS_COVER_DU		0x10000000 /* Cover directive, per design instance*/

/* ucisCoverageT stores values for a particular #define */
typedef struct {
    int     num_coveritems;	/* total number of coveritems (bins) */
    int     num_covered;	/* number of coveritems (bins) covered */
    float   coverage_pct;	/* floating point coverage value, percentage */
    int64_t weighted_numerator;
    int64_t weighted_denominator;
} ucisCoverageT;

/* ucisCoverageSummaryT stores all statistics returned by ucis_GetCoverageSummary() */

typedef struct {
    int num_instances;       /* number of design instances */
    int num_dus;             /* number of design units */
    int num_coverages;       /* number of elements in coverage */  
    ucisCoverageT *coverage;
} ucisCoverageSummaryT;


#define UCIS_SCORE_DEFAULT 0 /* vendor-specific default scoring algorithm */

float ucis_CoverageScore(
    ucisT db,
    ucisScopeT scope,
    int recursive,
    int scoring_algorithm,
    uint64_t metrics_mask,
    ucisCoverageSummaryT* data /* list only of types enabled in metrics_mask */
                               /* Ordering is lsb-first of metrics mask bits */
);

/*------------------------------------------------------------------------------
 *  Tags
 *
 *  A tag is a string associated with an object. An object may have multiple
 *  tags. The tag semantic is that things that share the same
 *  tag are associated together.
 * 
 *  Tags may be added to scopes, coveritems, and history nodes.
 *  See the ucis_ObjectTagsIterate()/ucis_ObjectTagsScan() routines for obtaining all
 *  the tags an object has.
 *  See the ucis_TaggedObjIterate()/ucis_TaggedObjScan() routines for iterating
 *  objects that have been tagged with a given tag.
 *
 *----------------------------------------------------------------------------*/

/*
 *  ucis_AddObjTag()
 *  Add a tag to a given obj.
 *  Returns 0 with success, non-zero with error.
 *  Error includes null tag or tag with '\n' character.
 */
int
ucis_AddObjTag(
    ucisT               db,
    ucisObjT            obj, /* ucisScopeT or ucisHistoryNodeT */
    const char*         tag);

/*
 *  ucis_RemoveObjTag()
 *  Remove the given tag from the obj.
 *  Returns 0 with success, non-zero with error.
 */
int
ucis_RemoveObjTag(
    ucisT               db,
    ucisObjT            obj, /* ucisScopeT or ucisHistoryNodeT */
    const char*         tag);

/*------------------------------------------------------------------------------
 *  Formal
 *
 *----------------------------------------------------------------------------*/

typedef void* ucisFormalEnvT;

/*
 * Formal Results enum
 *
 * The following enum is used in the API functions to indicate a formal result 
 * for an assertion:
 */
typedef enum {
    UCIS_FORMAL_NONE,         /* No formal info (default) */
    UCIS_FORMAL_FAILURE,      /* Fails */
    UCIS_FORMAL_PROOF,        /* Proven to never fail */
    UCIS_FORMAL_VACUOUS,      /* Assertion is vacuous as defined by the 
                                 assertion language */
    UCIS_FORMAL_INCONCLUSIVE, /* Proof failed to complete */
    UCIS_FORMAL_ASSUMPTION,   /* Assertion is an assume */
    UCIS_FORMAL_CONFLICT      /* Data merge conflict */
} ucisFormalStatusT;

typedef struct ucisFormalToolInfoS {
    char* formal_tool;
    char* formal_tool_version;
    char* formal_tool_setup;
    char* formal_tool_db;
    char* formal_tool_rpt;
    char* formal_tool_log;
} ucisFormalToolInfoT;

#define UCIS_FORMAL_COVERAGE_CONTEXT_STIMULUS \
    "UCIS_FORMAL_COVERAGE_CONTEXT_STIMULUS"
#define UCIS_FORMAL_COVERAGE_CONTEXT_RESPONSE \
    "UCIS_FORMAL_COVERAGE_CONTEXT_REPONSE"
#define UCIS_FORMAL_COVERAGE_CONTEXT_TARGETED \
    "UCIS_FORMAL_COVERAGE_CONTEXT_TARGETED"
#define UCIS_FORMAL_COVERAGE_CONTEXT_ANCILLARY \
    "UCIS_FORMAL_COVERAGE_CONTEXT_ANCILLARY"
#define UCIS_FORMAL_COVERAGE_CONTEXT_INCONCLUSIVE_ANALYSIS \
    "UCIS_FORMAL_COVERAGE_CONTEXT_INCONCLUSIVE_ANALYSIS"

int 
ucis_SetFormalStatus(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    ucisFormalStatusT formal_status);

int 
ucis_GetFormalStatus(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    ucisFormalStatusT* formal_status);

int 
ucis_SetFormalRadius(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    int radius,
    char* clock_name);

int 
ucis_GetFormalRadius(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    int* radius,
    char** clock);

int 
ucis_SetFormalWitness(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    char* witness_waveform_file_or_dir_name);

int 
ucis_GetFormalWitness(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT assertscope,
    char** witness_waveform_file_or_dir_name);

int 
ucis_SetFormallyUnreachableCoverTest(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT coverscope,
    int coverindex);

int 
ucis_ClearFormallyUnreachableCoverTest(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT coverscope,
    int coverindex);

int 
ucis_GetFormallyUnreachableCoverTest(
    ucisT db,
    ucisHistoryNodeT test,
    ucisScopeT coverscope,
    int coverindex,
    int* unreachable_flag);

ucisFormalEnvT 
ucis_AddFormalEnv(
    ucisT db,
    const char* name,
    ucisScopeT scope);

int 
ucis_FormalEnvGetData(
    ucisT db,
    ucisFormalEnvT formal_environment,
    const char** name,
    ucisScopeT* scope);

ucisFormalEnvT 
ucis_NextFormalEnv(
    ucisT db,
    ucisFormalEnvT formal_environment);

int 
ucis_AssocFormalInfoTest(
    ucisT db,
    ucisHistoryNodeT test,
    ucisFormalToolInfoT* formal_tool_info,
    ucisFormalEnvT formal_environment,
    char* formal_coverage_context);

int 
ucis_FormalTestGetInfo(
    ucisT db,
    ucisHistoryNodeT test,
    ucisFormalToolInfoT** formal_tool_info,
    ucisFormalEnvT* formal_environment,
    char ** formal_coverage_context);

int 
ucis_AssocAssumptionFormalEnv(
    ucisT db,
    ucisFormalEnvT formal_env,
    ucisScopeT assumption_scope);

ucisScopeT 
ucis_NextFormalEnvAssumption(
    ucisT db,
    ucisFormalEnvT formal_env,
    ucisScopeT assumption_scope);

/*------------------------------------------------------------------------------
 *  Miscellaneous
 *
 *----------------------------------------------------------------------------*/
/*
 *  ucis_GetFSMTransitionStates()
 *  Given a UCIS_FSM_TRANS coveritem, return the corresponding state coveritem
 *  indices and corresponding UCIS_FSM_STATES scope.
 *  This API removes the need to parse transition names in order to access
 *  the states.
 *  Returns the related scope of type UCIS_FSM_STATES on success, NULL on failure
 *  On success, the transition_index_list points to an integer array of 
 *  UCIS_FSM_STATES coverindexes in the transition order. The array is terminated
 *  with -1 and the array memory is valid until a subsequent call to this routine
 *  or closure of the database, whichever occurs first. Callers should not 
 *  attempt to free this list.
 */

ucisScopeT
ucis_GetFSMTransitionStates(
    ucisT      db,
    ucisScopeT trans_scope,             /* input handle for UCIS_FSM_TRANS scope */
    int        trans_index,             /* input coverindex for transition */
    int *      transition_index_list);  /* output array of int, -1 termination */

/*
 *  ucis_CreateNextTransition()
 *  This is a specialized version of ucis_CreateNextCover() to create a
 *  transition coveritem. It records the source and destination state
 *  coveritem indices along with the transition.
 *  The parent of the state coveritems is assumed to be a sibling of the parent
 *  of type UCIS_FSM_STATES.
 *  Returns the index number of the created coveritem, -1 if error.
 */
int
ucis_CreateNextTransition(
    ucisT db,
    ucisScopeT parent,            /* UCIS_FSM_TRANS scope */
    const char* name,             /* name of coveritem, can be NULL */
    ucisCoverDataT* data,         /* associated data for coverage */
    ucisSourceInfoT* sourceinfo,  /* can be NULL */
    int*  transition_index_list); /* input array of int, -1 termination */

#ifdef __cplusplus
}
#endif

#endif

