/*
 * Dynamic Windows:
 *          A GTK like GUI implementation of the Android GUI.
 *
 * (C) 2011-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2011-2022 Mark Hessling <mark@rexx.org>
 *
 * Requires Android API 26 (Oreo) or higher.
 *
 */

#include "dw.h"
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__ANDROID__) && (__ANDROID_API__+0) < 21
#include <sys/syscall.h>

/* Until Android API version 21 NDK does not define getsid wrapper in libc, although there is the corresponding syscall */
inline pid_t getsid(pid_t pid)
{
    return static_cast< pid_t >(::syscall(__NR_getsid, pid));
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DW_CLASS_NAME "org/dbsoft/dwindows/DWindows"

/* Dynamic Windows internal variables */
static char _dw_app_id[_DW_APP_ID_SIZE+1]= {0};
static char _dw_app_name[_DW_APP_ID_SIZE+1]= {0};
static char _dw_exec_dir[MAX_PATH+1] = {0};
static char _dw_user_dir[MAX_PATH+1] = {0};
static int _dw_android_api = 0;
static int _dw_container_mode = DW_CONTAINER_MODE_DEFAULT;
static int _dw_render_safe_mode = DW_FEATURE_ENABLED;

static pthread_key_t _dw_env_key;
static pthread_key_t _dw_fgcolor_key;
static pthread_key_t _dw_bgcolor_key;
static HEV _dw_main_event;
static JavaVM *_dw_jvm;
static jobject _dw_obj;
static jobject _dw_class_loader;
static jmethodID _dw_class_method;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved)
{
    JNIEnv *env;
    jclass randomClass, classClass, classLoaderClass;
    jmethodID getClassLoaderMethod;

    /* Initialize the handle to the Java Virtual Machine */
    _dw_jvm = pjvm;
    _dw_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);

    randomClass = env->FindClass(DW_CLASS_NAME);
    classClass = env->GetObjectClass(randomClass);
    classLoaderClass = env->FindClass("java/lang/ClassLoader");
    getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                                 "()Ljava/lang/ClassLoader;");
    _dw_class_loader = env->NewGlobalRef(env->CallObjectMethod(randomClass, getClassLoaderMethod));
    _dw_class_method = env->GetMethodID(classLoaderClass, "findClass",
                                        "(Ljava/lang/String;)Ljava/lang/Class;");

    return JNI_VERSION_1_6;
}

jclass _dw_find_class(JNIEnv *env, const char* name)
{
    jstring jname = env->NewStringUTF(name);
    jclass clazz = static_cast<jclass>(env->CallObjectMethod(_dw_class_loader, _dw_class_method, jname));
    env->DeleteLocalRef(jname);
    return clazz;
}

// Do a quick check if an exception occurred...
// Display then clear it so we can continue...
// Return TRUE if an exception had occurred
int _dw_jni_check_exception(JNIEnv *env)
{
    if(env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return TRUE;
    }
    return FALSE;
}

#define _DW_REFERENCE_NONE   0
#define _DW_REFERENCE_WEAK   1
#define _DW_REFERENCE_STRONG 2

// Handle returns from JNI functions and make sure the
// return value is sane...
// Reference: 0 no reference 1 weak 2 strong
jobject _dw_jni_check_result(JNIEnv *env, jobject obj, int reference)
{
    jobject result = nullptr;

    if(!_dw_jni_check_exception(env) && obj)
    {
        if(reference == _DW_REFERENCE_WEAK)
            result = env->NewWeakGlobalRef(obj);
        else if(reference == _DW_REFERENCE_STRONG)
            result = env->NewGlobalRef(obj);
        else
            result = obj;
    }
    return result;
}

/* Call the dwmain entry point, Android has no args, so just pass the app path */
void _dw_main_launch(char *arg)
{
    static HEV startup = nullptr;

    /* Safety check to prevent multiple initializations... */
    if(startup)
    {
        /* If we are called a second time.. post the event...
         * so we stop waiting.
         */
        dw_event_post(startup);
        return;
    }
    else
    {
        char *argv[2] = {arg, nullptr};

        /* Wait for a short while to see if we get called again...
         * if we get called again we will be posted, if not...
         * just wait for the timer to expire.
         */
        startup = dw_event_new();
        /* Wait for 10 seconds to see if we get called again */
        dw_event_wait(startup, 10000);

        /* Continue after being posted or after our timeout */
        dwmain(1, argv);
    }
    free(arg);
}

/* Called when DWindows activity starts up... so we create a thread
 *  to call the dwmain() entrypoint... then we wait for dw_main()
 *  to be called and return.
 * Parameters:
 *      path: The path to the Android app.
 */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_dwindowsInit(JNIEnv* env, jobject obj, jstring apppath, jstring appcache, jstring appID)
{
    const char *arg = env->GetStringUTFChars((jstring)apppath, nullptr);
    const char *cache = env->GetStringUTFChars((jstring)appcache, nullptr);
    const char *appid = env->GetStringUTFChars((jstring)appID, nullptr);
    char *home = getenv("HOME");

    if(!_dw_main_event)
    {
        /* Save our class object pointer for later */
        _dw_obj = env->NewGlobalRef(obj);

        /* Save the JNIEnv for the main thread */
        pthread_key_create(&_dw_env_key, nullptr);
        pthread_setspecific(_dw_env_key, env);
        pthread_key_create(&_dw_fgcolor_key, nullptr);
        pthread_setspecific(_dw_fgcolor_key, nullptr);
        pthread_key_create(&_dw_bgcolor_key, nullptr);
        pthread_setspecific(_dw_bgcolor_key, (void *)DW_RGB_TRANSPARENT);

        /* Create the dwmain event */
        _dw_main_event = dw_event_new();
    }

    if(cache)
    {
        /* Store the passed in path for dw_app_dir() */
        strncpy(_dw_exec_dir, cache, MAX_PATH);
        env->ReleaseStringUTFChars(appcache, cache);
    }
    /* Store the best path for dw_user_dir() */
    if(home)
        strncpy(_dw_user_dir, home, MAX_PATH);
    else if(arg)
        strncpy(_dw_user_dir, arg, MAX_PATH);
    else
        strcpy(_dw_user_dir, "/");
    if(appid)
    {
        /* Store our reported Android AppID */
        strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
        env->ReleaseStringUTFChars(appID, appid);
    }

    /* Launch the new thread to execute dwmain() */
    dw_thread_new((void *) _dw_main_launch, strdup(arg ? arg : ""), 0);

    if(arg)
        env->ReleaseStringUTFChars(apppath, arg);
}

typedef struct _dwsighandler
{
    struct _dwsighandler   *next;
    ULONG message;
    HWND window;
    int id;
    void *signalfunction;
    void *discfunction;
    void *data;

} DWSignalHandler;

static DWSignalHandler *DWRoot = nullptr;

DWSignalHandler *_dw_get_handler(HWND window, int messageid)
{
    DWSignalHandler *tmp = DWRoot;
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key))) {
        /* Find any callbacks for this function */
        while (tmp) {
            if (tmp->message == messageid && env->IsSameObject(window, tmp->window)) {
                return tmp;
            }
            tmp = tmp->next;
        }
    }
    return nullptr;
}

typedef struct
{
    ULONG message;
    char name[30];

} DWSignalList;

/* List of signals */
static DWSignalList DWSignalTranslate[] = {
        { _DW_EVENT_CONFIGURE,      DW_SIGNAL_CONFIGURE },
        { _DW_EVENT_KEY_PRESS,      DW_SIGNAL_KEY_PRESS },
        { _DW_EVENT_BUTTON_PRESS,   DW_SIGNAL_BUTTON_PRESS },
        { _DW_EVENT_BUTTON_RELEASE, DW_SIGNAL_BUTTON_RELEASE },
        { _DW_EVENT_MOTION_NOTIFY,  DW_SIGNAL_MOTION_NOTIFY },
        { _DW_EVENT_DELETE,         DW_SIGNAL_DELETE },
        { _DW_EVENT_EXPOSE,         DW_SIGNAL_EXPOSE },
        { _DW_EVENT_CLICKED,        DW_SIGNAL_CLICKED },
        { _DW_EVENT_ITEM_ENTER,     DW_SIGNAL_ITEM_ENTER },
        { _DW_EVENT_ITEM_CONTEXT,   DW_SIGNAL_ITEM_CONTEXT },
        { _DW_EVENT_LIST_SELECT,    DW_SIGNAL_LIST_SELECT },
        { _DW_EVENT_ITEM_SELECT,    DW_SIGNAL_ITEM_SELECT },
        { _DW_EVENT_SET_FOCUS,      DW_SIGNAL_SET_FOCUS },
        { _DW_EVENT_VALUE_CHANGED,  DW_SIGNAL_VALUE_CHANGED },
        { _DW_EVENT_SWITCH_PAGE,    DW_SIGNAL_SWITCH_PAGE },
        { _DW_EVENT_TREE_EXPAND,    DW_SIGNAL_TREE_EXPAND },
        { _DW_EVENT_COLUMN_CLICK,   DW_SIGNAL_COLUMN_CLICK },
        { _DW_EVENT_HTML_RESULT,    DW_SIGNAL_HTML_RESULT },
        { _DW_EVENT_HTML_CHANGED,   DW_SIGNAL_HTML_CHANGED },
        { _DW_EVENT_HTML_MESSAGE,   DW_SIGNAL_HTML_MESSAGE },
        { 0,                        "" }
};

#define _DW_EVENT_PARAM_SIZE 10

void _dw_render_create_bitmap(HWND handle);
void _dw_render_finish_draw(HWND handle);


int _dw_event_handler2(void **params)
{
    DWSignalHandler *handler = (DWSignalHandler *)params[9];
    int message = DW_POINTER_TO_INT(params[8]);
    int retval = -1;

    if(handler)
    {
        switch(message)
        {
            /* Timer event */
            case _DW_EVENT_TIMER:
            {
                int (*timerfunc)(void *) = (int (* API)(void *))handler->signalfunction;

                if(!timerfunc(handler->data))
                    dw_timer_disconnect(handler->window);
                retval = 0;
                break;
            }
                /* Configure/Resize event */
            case _DW_EVENT_CONFIGURE:
            {
                int (*sizefunc)(HWND, int, int, void *) = (int (* API)(HWND, int, int, void *))handler->signalfunction;
                int width = DW_POINTER_TO_INT(params[3]);
                int height = DW_POINTER_TO_INT(params[4]);

                if(width > 0 && height > 0)
                    retval = sizefunc(handler->window, width, height, handler->data);
                else
                    retval = 0;
                break;
            }
            case _DW_EVENT_KEY_PRESS:
            {
                int (*keypressfunc)(HWND, char, int, int, void *, char *) = (int (* API)(HWND, char, int, int, void *, char *))handler->signalfunction;
                char *utf8 = (char *)params[1], ch = (char)DW_POINTER_TO_INT(params[3]);
                int vk = DW_POINTER_TO_INT(params[4]), special = DW_POINTER_TO_INT(params[5]);

                retval = keypressfunc(handler->window, ch, (int)vk, special, handler->data, utf8);
                if(utf8)
                    free(utf8);
                break;
            }
                /* Button press and release event */
            case _DW_EVENT_BUTTON_PRESS:
            case _DW_EVENT_BUTTON_RELEASE:
            {
                int (* API buttonfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                int button = params[5] ? DW_POINTER_TO_INT(params[5]) : 1;

                retval = buttonfunc(handler->window, DW_POINTER_TO_INT(params[3]), DW_POINTER_TO_INT(params[4]), button, handler->data);
                break;
            }
            /* Motion notify event */
            case _DW_EVENT_MOTION_NOTIFY:
            {
                int (* API motionfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;

                retval = motionfunc(handler->window, DW_POINTER_TO_INT(params[3]), DW_POINTER_TO_INT(params[4]), DW_POINTER_TO_INT(params[5]), handler->data);
                break;
            }
            /* Window close event */
            case _DW_EVENT_DELETE:
            {
                int (* API closefunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;
                retval = closefunc(handler->window, handler->data);
                if(retval < 1)
                    dw_window_destroy(handler->window);
                break;
            }
            /* Window expose/draw event */
            case _DW_EVENT_EXPOSE:
            {
                DWExpose exp;
                int (* API exposefunc)(HWND, DWExpose *, void *) = (int (* API)(HWND, DWExpose *, void *))handler->signalfunction;

                exp.x = DW_POINTER_TO_INT(params[3]);
                exp.y = DW_POINTER_TO_INT(params[4]);
                exp.width = DW_POINTER_TO_INT(params[5]);
                exp.height = DW_POINTER_TO_INT(params[6]);
                if(_dw_render_safe_mode)
                    _dw_render_create_bitmap(handler->window);
                retval = exposefunc(handler->window, &exp, handler->data);
                if(_dw_render_safe_mode)
                    _dw_render_finish_draw(handler->window);
                /* Return here so we don't free params since we
                 * are always handling expose/draw on the UI thread.
                 */
                return retval;
            }
                /* Clicked event for buttons and menu items */
            case _DW_EVENT_CLICKED:
            {
                int (* API clickfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                retval = clickfunc(handler->window, handler->data);
                break;
            }
                /* Container class selection event */
            case _DW_EVENT_ITEM_ENTER:
            {
                int (*containerselectfunc)(HWND, char *, void *, void *) =(int (* API)(HWND, char *, void *, void *)) handler->signalfunction;
                char *text = (char *)params[1];

                retval = containerselectfunc(handler->window, text, handler->data, params[7]);
                if(text)
                    free(text);
                break;
            }
                /* Container context menu event */
            case _DW_EVENT_ITEM_CONTEXT:
            {
                int (* API containercontextfunc)(HWND, char *, int, int, void *, void *) = (int (* API)(HWND, char *, int, int, void *, void *))handler->signalfunction;
                char *text = (char *)params[1];
                void *user = params[7];
                int x = DW_POINTER_TO_INT(params[3]);
                int y = DW_POINTER_TO_INT(params[4]);

                retval = containercontextfunc(handler->window, text, x, y, handler->data, user);
                if(text)
                    free(text);
                break;
            }
                /* Generic selection changed event for several classes */
            case _DW_EVENT_LIST_SELECT:
            case _DW_EVENT_VALUE_CHANGED:
            {
                int (* API valuechangedfunc)(HWND, int, void *) = (int (* API)(HWND, int, void *))handler->signalfunction;
                int selected = DW_POINTER_TO_INT(params[3]);

                retval = valuechangedfunc(handler->window, selected, handler->data);
                break;
            }
                /* Tree class selection event */
            case _DW_EVENT_ITEM_SELECT:
            {
                int (* API treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = (int (* API)(HWND, HTREEITEM, char *, void *, void *))handler->signalfunction;
                char *text = (char *)params[1];
                void *user = params[7];

                retval = treeselectfunc(handler->window, (jobject)params[0], text, handler->data, user);
                if(text)
                    free(text);
                break;
            }
                /* Set Focus event */
            case _DW_EVENT_SET_FOCUS:
            {
                int (* API setfocusfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                retval = setfocusfunc(handler->window, handler->data);
                break;
            }
                /* Notebook page change event */
            case _DW_EVENT_SWITCH_PAGE:
            {
                int (* API switchpagefunc)(HWND, unsigned long, void *) = (int (* API)(HWND, unsigned long, void *))handler->signalfunction;
                unsigned long pageID = DW_POINTER_TO_INT(params[3]);

                retval = switchpagefunc(handler->window, pageID, handler->data);
                break;
            }
                /* Tree expand event */
            case _DW_EVENT_TREE_EXPAND:
            {
                int (* API treeexpandfunc)(HWND, HTREEITEM, void *) = (int (* API)(HWND, HTREEITEM, void *))handler->signalfunction;

                retval = treeexpandfunc(handler->window, (HTREEITEM)params[0], handler->data);
                break;
            }
                /* Column click event */
            case _DW_EVENT_COLUMN_CLICK:
            {
                int (* API clickcolumnfunc)(HWND, int, void *) = (int (* API)(HWND, int, void *))handler->signalfunction;
                int column_num = DW_POINTER_TO_INT(params[3]);

                retval = clickcolumnfunc(handler->window, column_num, handler->data);
                break;
            }
                /* HTML result event */
            case _DW_EVENT_HTML_RESULT:
            {
                int (* API htmlresultfunc)(HWND, int, char *, void *, void *) = (int (* API)(HWND, int, char *, void *, void *))handler->signalfunction;
                char *result = (char *)params[1];

                retval = htmlresultfunc(handler->window, result ? DW_ERROR_NONE : DW_ERROR_UNKNOWN, result, params[7], handler->data);
                if(result)
                    free(result);
                break;
            }
                /* HTML changed event */
            case _DW_EVENT_HTML_CHANGED:
            {
                int (* API htmlchangedfunc)(HWND, int, char *, void *) = (int (* API)(HWND, int, char *, void *))handler->signalfunction;
                char *uri = (char *)params[1];

                retval = htmlchangedfunc(handler->window, DW_POINTER_TO_INT(params[3]), uri, handler->data);
                if(uri)
                    free(uri);
                break;
            }
                /* HTML message event */
            case _DW_EVENT_HTML_MESSAGE:
            {
                int (* API htmlmessagefunc)(HWND, char *, char *, void *) = (int (* API)(HWND, char *, char *, void *))handler->signalfunction;
                char *name = (char *)params[1];
                char *body = (char *)params[2];

                retval = htmlmessagefunc(handler->window, name, body, handler->data);
                if(name)
                    free(name);
                if(body)
                    free(body);
                break;
            }
        }
    }
    return retval;
}

/* We create queues of events... the oldest event indexed by
 * _dw_event_queue->head, the newest by _dw_event_queue->tail.
 */
#define _DW_EVENT_QUEUE_LENGTH 50

typedef struct
{
    int head, tail;
    HMTX mutex;
    void *queue[_DW_EVENT_QUEUE_LENGTH][_DW_EVENT_PARAM_SIZE];
} _dw_event_queue;

/* Higher priority draw queue gets handled first */
_dw_event_queue _dw_high = { -1, -1 };
_dw_event_queue _dw_low = { -1, -1 };
int _dw_main_active = TRUE;
DWTID _dw_main_thread = -1;

/* Add a new event to the queue if there is space.
 * This will be handled in the thread running dw_main()
 */
int _dw_queue_event(void **params, _dw_event_queue *queue)
{
    int newtail = queue->tail + 1;
    int retval = FALSE;

    /* Initialize the mutex if necessary... return on failure. */
    if(!queue->mutex && !(queue->mutex = dw_mutex_new()))
        return retval;

    /* Protect the queue in a mutex... hold for as short as possible */
    while(dw_mutex_trylock(queue->mutex) != DW_ERROR_NONE)
        sched_yield();

    /* If we are at the end of the queue, loop back to the start. */
    if(newtail >= _DW_EVENT_QUEUE_LENGTH)
        newtail = 0;
    /* If the new tail will be at the head, the event queue
     * if full... drop the event.
     */
    if(newtail != queue->head)
    {
        /* If the queue was empty, head and tail will be the same. */
        if (queue->head == -1)
            queue->head = newtail;
        /* Copy the new event from the stack into the event queue. */
        memcpy(&queue->queue[newtail], params, _DW_EVENT_PARAM_SIZE * sizeof(void *));
        /* Update the tail index */
        queue->tail = newtail;
        /* Successfully queued event */
        retval = TRUE;
    }
    dw_mutex_unlock(queue->mutex);
    return retval;
}

/* If there is an event waiting, pop it off the queue,
 * advance the head and return TRUE.
 * If there are no events waiting return FALSE
 */
int _dw_dequeue_event(void **params, _dw_event_queue *queue)
{
    int retval = FALSE;

    /* Initialize the mutex if necessary... return FALSE on failure. */
    if(!queue->mutex && !(queue->mutex = dw_mutex_new()))
        return retval;
    dw_mutex_lock(queue->mutex);
    if(queue->head != -1)
    {
        /* Copy the params out of the queue so it can be filled in by new events */
        memcpy(params, &queue->queue[queue->head], _DW_EVENT_PARAM_SIZE * sizeof(void *));

        /* If the head is the same as the tail...
         * there was only one event... so set the
         * head and tail to -1 to indicate empty.
         */
        if(queue->head == queue->tail)
            queue->head = queue->tail = -1;
        else
        {
            /* Advance the head */
            queue->head++;

            /* If we are at the end of the queue, loop back to the start. */
            if(queue->head >= _DW_EVENT_QUEUE_LENGTH)
                queue->head = 0;
        }
        /* Successfully dequeued event */
        retval = TRUE;
        /* Notify dw_main() that there is an event to handle */
        dw_event_post(_dw_main_event);
    }
    dw_mutex_unlock(queue->mutex);
    return retval;
}

int _dw_event_handler(jobject object, void **params)
{
    int messageid = DW_POINTER_TO_INT(params[8]);
    DWSignalHandler *handler = _dw_get_handler(object, messageid);

    if (handler)
    {
        params[9] = (void *)handler;

        /* When we are not in safe rendering mode,
         * We have to handle draw events in the main thread.
         * If it isn't a draw event, queue the event.
         */
        if(DW_POINTER_TO_INT(params[8]) != _DW_EVENT_EXPOSE)
        {
            /* Push the new event onto the queue if it fits */
            _dw_queue_event(params, &_dw_high);
        }
        else if(_dw_render_safe_mode == DW_FEATURE_ENABLED)
        {
            /* Push the new event onto the high priority queue if it fits */
            _dw_queue_event(params, &_dw_low);
        }
        else
            return _dw_event_handler2(params);

    } /* If we don't have a handler, destroy the window */
    else if(messageid == _DW_EVENT_DELETE)
        dw_window_destroy(object);
    return 0;
}

/*
 * Entry location for all event handlers from the Android UI
 */
JNIEXPORT jint JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandler(JNIEnv* env, jobject obj, jobject obj1, jobject obj2,
                                                      jint message, jstring str1, jstring str2,
                                                      jint inta, jint intb, jint intc, jint intd) {
    const char *utf81 = str1 ? env->GetStringUTFChars(str1, nullptr) : nullptr;
    const char *utf82 = str2 ? env->GetStringUTFChars(str2, nullptr) : nullptr;
    void *params[_DW_EVENT_PARAM_SIZE] = { DW_POINTER(obj2), DW_POINTER(utf81 ? strdup(utf81) : nullptr),
                                           DW_POINTER(utf82 ? strdup(utf82) : nullptr),
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    int retval = _dw_event_handler(obj1, params);
    if(utf81)
        env->ReleaseStringUTFChars(str1, utf81);
    if(utf82)
        env->ReleaseStringUTFChars(str2, utf82);
    return retval;
}

/* A more simple method for quicker calls */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerSimple(JNIEnv* env, jobject obj, jobject obj1, jint message)
{
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr };

    params[8] = DW_INT_TO_POINTER(message);
    _dw_event_handler(obj1, params);
}

/* Handler for notebook page changes */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerNotebook(JNIEnv* env, jobject obj, jobject obj1, jint message, jlong pageID)
{
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr };

    params[3] = DW_INT_TO_POINTER(pageID);
    params[8] = DW_INT_TO_POINTER(message);
    _dw_event_handler(obj1, params);
}

/* Handlers for HTML events */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerHTMLResult(JNIEnv* env, jobject obj, jobject obj1,
                                               jint message, jstring htmlResult, jlong data) {
    const char *result = env->GetStringUTFChars(htmlResult, nullptr);
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, DW_POINTER(result ? strdup(result) : nullptr), nullptr,
                                           nullptr, nullptr, nullptr, nullptr, DW_INT_TO_POINTER(data),
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
    if(result)
        env->ReleaseStringUTFChars(htmlResult, result);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWWebViewClient_eventHandlerHTMLChanged(JNIEnv* env, jobject obj, jobject obj1,
                                                         jint message, jstring URI, jint status) {
    const char *uri = env->GetStringUTFChars(URI, nullptr);
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, DW_POINTER(uri ? strdup(uri) : nullptr), nullptr,
                                           DW_INT_TO_POINTER(status), nullptr, nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
    if(uri)
        env->ReleaseStringUTFChars(URI, uri);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWWebViewInterface_eventHandlerHTMLMessage(JNIEnv *env, jobject thiz,
                                                                    jobject obj1, jint message,
                                                                    jstring htmlName, jstring htmlBody) {
    const char *name = env->GetStringUTFChars(htmlName, nullptr);
    const char *body = env->GetStringUTFChars(htmlBody, nullptr);
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, DW_POINTER(name ? strdup(name) : nullptr),
                                           DW_POINTER(body ? strdup(body) : nullptr),
                                           nullptr, nullptr, nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    // This seems callback seems to run on an uninitialized thread...
    pthread_setspecific(_dw_env_key, env);

    _dw_event_handler(obj1, params);
    if(name)
        env->ReleaseStringUTFChars(htmlName, name);
    if(body)
        env->ReleaseStringUTFChars(htmlBody, body);
}


typedef struct _dwprint
{
    int (*drawfunc)(HPRINT, HPIXMAP, int, void *);
    void *drawdata;
    unsigned long flags;
    char *jobname;
    jint pages;
    jobject printjob;
} DWPrint;

/* Handlers for Print events */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWPrintDocumentAdapter_eventHandlerPrintDraw(JNIEnv* env, jobject obj, jlong pr,
                                                                      jobject bitmap, jint page, jint width, jint height)
{
    DWPrint *print = (DWPrint *)pr;

    if(print && print->drawfunc)
    {
        HPIXMAP pixmap = (HPIXMAP)alloca(sizeof(struct _hpixmap));

        memset(pixmap, 0, sizeof(struct _hpixmap));
        pixmap->width = width;
        pixmap->height = height;
        pixmap->bitmap = bitmap;

        print->drawfunc(print, pixmap, (int)page, print->drawdata);
    }
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWPrintDocumentAdapter_eventHandlerPrintFinish(JNIEnv* env, jobject obj, jlong pr)
{
    DWPrint *print = (DWPrint *) pr;

    if(print)
    {
        if(print->jobname)
            free(print->jobname);
        free(print);
    }
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerInt(JNIEnv* env, jobject obj, jobject obj1, jint message,
                                               jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWComboBox_eventHandlerInt(JNIEnv* env, jobject obj, jint message,
                                                  jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWSpinButton_eventHandlerInt(JNIEnv* env, jobject obj, jint message,
                                                    jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWListBox_eventHandlerInt(JNIEnv* env, jobject obj, jint message,
                                                    jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWRender_eventHandlerInt(JNIEnv* env, jobject obj, jint message,
                                                   jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWContainerAdapter_eventHandlerInt(JNIEnv* env, jobject obj, jobject obj1,
                                                            jint message, jint inta, jint intb, jint intc, jint intd) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, nullptr, nullptr,
                                           DW_INT_TO_POINTER(inta), DW_INT_TO_POINTER(intb),
                                           DW_INT_TO_POINTER(intc), DW_INT_TO_POINTER(intd), nullptr,
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerContainer(JNIEnv* env, jobject obj, jobject obj1,
                                                  jint message, jstring jtitle, jint x, jint y, jlong data) {
    const char *title = jtitle ? env->GetStringUTFChars(jtitle, nullptr) : nullptr;
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, DW_POINTER(title ? strdup(title) : nullptr), nullptr,
                                           DW_INT_TO_POINTER(x), DW_INT_TO_POINTER(y), nullptr, nullptr,
                                           DW_POINTER(data), DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
    if(title)
        env->ReleaseStringUTFChars(jtitle, title);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerTree(JNIEnv* env, jobject obj, jobject obj1,
                                                   jint message, jobject item, jstring jtitle, jlong data) {
    const char *title = jtitle ? env->GetStringUTFChars(jtitle, nullptr) : nullptr;
    void *params[_DW_EVENT_PARAM_SIZE] = { DW_POINTER(item), DW_POINTER(title ? strdup(title) : nullptr),
                                           nullptr, nullptr, nullptr, nullptr, nullptr, DW_POINTER(data),
                                           DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
    if(title)
        env->ReleaseStringUTFChars(jtitle, title);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerTreeItem(JNIEnv* env, jobject obj, jobject obj1,
                                                   jint message, jobject item) {
    void *params[_DW_EVENT_PARAM_SIZE] = { DW_POINTER(item), nullptr, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
}

JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerKey(JNIEnv *env, jobject obj, jobject obj1, jint message, jint ch,
                                                  jint vk, jint modifiers, jstring str) {
    const char *cstr = str ? env->GetStringUTFChars(str, nullptr) : nullptr;
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr, DW_POINTER(cstr ? strdup(cstr) : nullptr), nullptr,
                                           DW_INT_TO_POINTER(ch), DW_INT_TO_POINTER(vk), DW_INT_TO_POINTER(modifiers),
                                           nullptr, nullptr, DW_INT_TO_POINTER(message), nullptr };

    _dw_event_handler(obj1, params);
    if(cstr)
        env->ReleaseStringUTFChars(str, cstr);
}

/* Handler for Timer events */
JNIEXPORT jint JNICALL
Java_org_dbsoft_dwindows_DWindows_eventHandlerTimer(JNIEnv* env, jobject obj, jlong sigfunc, jlong data) {
    int (*timerfunc)(void *) = (int (* API)(void *))sigfunc;

    pthread_setspecific(_dw_env_key, env);
    return timerfunc((void *)data);
}

/* A more simple method for quicker calls */
JNIEXPORT void JNICALL
Java_org_dbsoft_dwindows_DWMenu_eventHandlerSimple(JNIEnv* env, jobject obj, jobject obj1, jint message) {
    void *params[_DW_EVENT_PARAM_SIZE] = { nullptr };

    params[8] = DW_INT_TO_POINTER(message);
    _dw_event_handler(obj1, params);
}


/* This function adds a signal handler callback into the linked list.
 */
void _dw_new_signal(ULONG message, HWND window, int msgid, void *signalfunction, void *discfunc, void *data)
{
    DWSignalHandler *newsig = (DWSignalHandler *)malloc(sizeof(DWSignalHandler));

    newsig->message = message;
    newsig->window = window;
    newsig->id = msgid;
    newsig->signalfunction = signalfunction;
    newsig->discfunction = discfunc;
    newsig->data = data;
    newsig->next = nullptr;

    if (!DWRoot)
        DWRoot = newsig;
    else
    {
        DWSignalHandler *prev = nullptr, *tmp = DWRoot;
        while(tmp)
        {
            if(tmp->message == message &&
               tmp->window == window &&
               tmp->id == msgid &&
               tmp->signalfunction == signalfunction)
            {
                tmp->data = data;
                free(newsig);
                return;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if(prev)
            prev->next = newsig;
        else
            DWRoot = newsig;
    }
}

/* Finds the message number for a given signal name */
ULONG _dw_findsigmessage(const char *signame)
{
    int z = 0;

    while(DWSignalTranslate[z].message)
    {
        if(strcasecmp(signame, DWSignalTranslate[z].name) == 0)
            return DWSignalTranslate[z].message;
        z++;
    }
    return 0L;
}

unsigned long _dw_colors[] =
{
        0x00000000,   /* 0  black */
        0x000000bb,   /* 1  red */
        0x0000bb00,   /* 2  green */
        0x0000aaaa,   /* 3  yellow */
        0x00cc0000,   /* 4  blue */
        0x00bb00bb,   /* 5  magenta */
        0x00bbbb00,   /* 6  cyan */
        0x00bbbbbb,   /* 7  white */
        0x00777777,   /* 8  grey */
        0x000000ff,   /* 9  bright red */
        0x0000ff00,   /* 10 bright green */
        0x0000eeee,   /* 11 bright yellow */
        0x00ff0000,   /* 12 bright blue */
        0x00ff00ff,   /* 13 bright magenta */
        0x00eeee00,   /* 14 bright cyan */
        0x00ffffff,   /* 15 bright white */
        0xff000000    /* 16 default color */
};

/* Return the RGB color regardless if a predefined color was passed */
jlong _dw_get_color(unsigned long thiscolor)
{
    if(thiscolor & DW_RGB_COLOR)
    {
        return ((jlong)thiscolor) & ~DW_RGB_COLOR;
    }
    else if(thiscolor < 17)
    {
        return _dw_colors[thiscolor];
    }
    else if(thiscolor == DW_RGB_TRANSPARENT)
    {
        return -1;
    }
    return 0;
}

int _dw_dark_mode_detected()
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID darkModeDetected = env->GetMethodID(clazz, "darkModeDetected",
                                                      "()I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, darkModeDetected);
        _dw_jni_check_exception(env);
    }
    return retval;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID dwMain = env->GetMethodID(clazz, "dwMain",
                                            "()V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, dwMain);
        _dw_jni_check_exception(env);
    }

    /* Save our thread ID, so we know if we should handle
     * events in callback dw_main_sleep/iteration() calls.
     */
    _dw_main_thread = dw_thread_id();

    do
    {
        void *params[_DW_EVENT_PARAM_SIZE];
        int result;

        dw_event_reset(_dw_main_event);

        /* Dequeue and handle any pending events,
         * always checking for high events first.
         */
        do
        {
            if((result = _dw_dequeue_event(params, &_dw_high)))
                _dw_event_handler2(params);
            else if((result = _dw_dequeue_event(params, &_dw_low)))
                _dw_event_handler2(params);
        } while(result);

        /* Wait for something to wake us up,
         * either a posted event, or dw_main_quit()
         */
        dw_event_wait(_dw_main_event, 100);

    } while(_dw_main_active);
    _dw_main_thread = -1;
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
    _dw_main_active = FALSE;
    dw_event_post(_dw_main_event);
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
    JNIEnv *env;

    /* If we are in an event callback from dw_main() ...
     * we need to continue handling events from the UI.
     */
    if(_dw_main_thread == dw_thread_id())
    {
        struct timeval tv, start;
        /* The time left to wait from the start */
        int difference = milliseconds;

        gettimeofday(&start, NULL);

        do
        {
            void *params[_DW_EVENT_PARAM_SIZE];
            int result;

            dw_event_reset(_dw_main_event);

            /* Dequeue and handle any pending events,
             * always checking for high events first.
             */
            do
            {
                if((result = _dw_dequeue_event(params, &_dw_high)))
                    _dw_event_handler2(params);
                else if((result = _dw_dequeue_event(params, &_dw_low)))
                    _dw_event_handler2(params);
            } while(result);

            /* Wait for something to wake us up,
             * either a posted event, or dw_main_quit()
             */
            dw_event_wait(_dw_main_event, difference);

            gettimeofday(&tv, NULL);

        } while(_dw_main_active && (difference = ((tv.tv_sec - start.tv_sec)*1000) + ((tv.tv_usec - start.tv_usec)/1000)) <= milliseconds);
    }
    else if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mainSleep = env->GetMethodID(clazz, "mainSleep",
                                                  "(I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mainSleep, milliseconds);
        _dw_jni_check_exception(env);
    }
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
    if(_dw_main_thread == dw_thread_id())
    {
        void *params[_DW_EVENT_PARAM_SIZE];

        /* Dequeue a single pending event, try high first */
        if (_dw_dequeue_event(params, &_dw_high))
            _dw_event_handler2(params);
        else if (_dw_dequeue_event(params, &_dw_low))
            _dw_event_handler2(params);
    }
    else
    /* If we sleep for 0 milliseconds... we will drop out
     * of the loop at the first idle moment
     */
        dw_main_sleep(0);
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID dwindowsExit = env->GetMethodID(clazz, "dwindowsExit",
                                                  "(I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, dwindowsExit, exitcode);
        _dw_jni_check_exception(env);
    }
    // We shouldn't get here, but in case JNI can't call dwindowsExit...
    exit(exitcode);
}

/*
 * Free's memory allocated by dynamic windows.
 * Parameters:
 *           ptr: Pointer to dynamic windows allocated
 *                memory to be free()'d.
 */
void API dw_free(void *ptr)
{
    free(ptr);
}

/*
 * Returns a pointer to a static buffer which contains the
 * current user directory.  Or the root directory if it could
 * not be determined.
 */
char * API dw_user_dir(void)
{
    return _dw_user_dir;
}

/*
 * Returns a pointer to a static buffer which contains the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    /* The path is passed in via JNI dwindowsInit() */
    return _dw_exec_dir;
}

/*
 * Sets the application ID used by this Dynamic Windows application instance.
 * Parameters:
 *         appid: A string typically in the form: com.company.division.application
 *         appname: The application name used on Windows or nullptr.
 * Returns:
 *         DW_ERROR_NONE after successfully setting the application ID.
 *         DW_ERROR_UNKNOWN if unsupported on this system.
 *         DW_ERROR_GENERAL if the application ID is not allowed.
 * Remarks:
 *          This must be called before dw_init().  If dw_init() is called first
 *          it will create a unique ID in the form: org.dbsoft.dwindows.application
 *          or if the application name cannot be detected: org.dbsoft.dwindows.pid.#
 *          The appname is used on Windows and Android.  If nullptr is passed the
 *          detected name will be used, but a prettier name may be desired.
 */
int API dw_app_id_set(const char *appid, const char *appname)
{
    if(appid)
        strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
    if(appname)
        strncpy(_dw_app_name, appname, _DW_APP_ID_SIZE);
    return DW_ERROR_NONE;
}

/*
 * Displays a debug message on the console...
 * Parameters:
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
void API dw_debug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        char outbuf[1025] = {0};

        vsnprintf(outbuf, 1024, format, args);

        // Construct a String
        jstring jstr = env->NewStringUTF(outbuf);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID debugMessage = env->GetMethodID(clazz, "debugMessage",
                                               "(Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, debugMessage, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
    else {
        /* Output to stderr, if there is another way to send it
         * on the implementation platform, change this.
         */
        vfprintf(stderr, format, args);
    }
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: flags to indicate buttons and icon
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 * Returns:
 *       DW_MB_RETURN_YES, DW_MB_RETURN_NO, DW_MB_RETURN_OK,
 *       or DW_MB_RETURN_CANCEL based on flags and user response.
 */
int API dw_messagebox(const char *title, int flags, const char *format, ...)
{
    va_list args;
    int rc;

    va_start(args, format);
    rc = dw_vmessagebox(title, flags, format, args);
    va_end(args);
    return rc;
}

int API dw_vmessagebox(const char *title, int flags, const char *format, va_list args)
{
    JNIEnv *env;
    int retval = 0;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        char outbuf[1025] = {0};

        vsnprintf(outbuf, 1024, format, args);

        // Construct a String
        jstring jstr = env->NewStringUTF(outbuf);
        jstring jstrtitle = env->NewStringUTF(title);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID messageBox = env->GetMethodID(clazz, "messageBox",
                                                  "(Ljava/lang/String;Ljava/lang/String;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, messageBox, jstrtitle, jstr, flags);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
        env->DeleteLocalRef(jstrtitle);
    }
    return retval;
}

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extention.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE.
 * Returns:
 *       nullptr on error. A malloced buffer containing
 *       the file path on success.
 *
 */
char * API dw_file_browse(const char *title, const char *defpath, const char *ext, int flags)
{
    JNIEnv *env;
    char *retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Use a long parameter
        jstring jstr = env->NewStringUTF(title);
        jstring path = nullptr;
        jstring jext = nullptr;
        if(defpath)
            path = env->NewStringUTF(defpath);
        if(ext)
            jext = env->NewStringUTF(defpath);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID fileBrowse = env->GetMethodID(clazz, "fileBrowseNew",
                                                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;");
        // Call the method on the object
        jstring jresult = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, fileBrowse, jstr, path, jext, flags), _DW_REFERENCE_NONE);
        if(jresult)
        {
            const char *str = env->GetStringUTFChars(jresult, nullptr);
            if(str)
                retval = strdup(str);
        }
        env->DeleteLocalRef(jstr);
        if(path)
            env->DeleteLocalRef(path);
        if(jext)
            env->DeleteLocalRef(jext);
    }
    return retval;
}

int API dw_file_open(const char *path, int mode)
{
    int retval = -1;

    if(path)
    {
        JNIEnv *env;

        // If we have no URI we can use the normal open call
        if(!strstr(path, "://"))
            return open(path, mode);
        // If we have a URI, use Android calls to get the URI file descriptor
        if ((env = (JNIEnv *) pthread_getspecific(_dw_env_key)))
        {
            // dw_file_browse saves a second string with the URI after the path
            // So find the end of the string and check for a trailing \n
            // The URI will be after that if found.
            jstring jstr = env->NewStringUTF(path);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID fileOpen = env->GetMethodID(clazz, "fileOpen",
                                                  "(Ljava/lang/String;I)I");
            // Call the method on the object
            retval = (int) env->CallIntMethod(_dw_obj, fileOpen, jstr, (jint) mode);
            _dw_jni_check_exception(env);
            env->DeleteLocalRef(jstr);
        }
    }
    return retval;
}

/*
 * Gets the contents of the default clipboard as text.
 * Parameters:
 *       None.
 * Returns:
 *       Pointer to an allocated string of text or nullptr if clipboard empty or contents could not
 *       be converted to text.
 */
char * API dw_clipboard_get_text()
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        const char *utf8 = nullptr;
        char *retval = nullptr;

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID clipboardGetText = env->GetMethodID(clazz, "clipboardGetText",
                                                      "()Ljava/lang/String;");
        // Call the method on the object
        jstring result = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, clipboardGetText), _DW_REFERENCE_NONE);
        // Get the UTF8 string result
        if(result)
            utf8 = env->GetStringUTFChars(result, nullptr);
        if(utf8)
        {
            retval = strdup(utf8);
            env->ReleaseStringUTFChars(result, utf8);
        }
        return retval;
    }
    return nullptr;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       str: Text to put on the clipboard.
 *       len: Length of the text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(str);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID clipboardSetText = env->GetMethodID(clazz, "clipboardSetText",
                                                      "(Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, clipboardSetText, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 * Returns:
 *       A handle to a dialog or nullptr on failure.
 */
DWDialog * API dw_dialog_new(void *data)
{
    DWDialog *tmp = (DWDialog *)malloc(sizeof(DWDialog));

    if(tmp)
    {
        tmp->eve = dw_event_new();
        dw_event_reset(tmp->eve);
        tmp->data = data;
        tmp->done = FALSE;
        tmp->result = nullptr;
    }
    return tmp;
}

/*
 * Accepts a dialog struct and returns the given data to the
 * initial called of dw_dialog_wait().
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 *           result: Data to be returned by dw_dialog_wait().
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_dialog_dismiss(DWDialog *dialog, void *result)
{
    dialog->result = result;
    dw_event_post(dialog->eve);
    dialog->done = TRUE;
    return DW_ERROR_NONE;
}

/*
 * Accepts a dialog struct waits for dw_dialog_dismiss() to be
 * called by a signal handler with the given dialog struct.
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 * Returns:
 *       The data passed to dw_dialog_dismiss().
 */
void * API dw_dialog_wait(DWDialog *dialog)
{
    void *tmp = nullptr;

    while(!dialog->done)
    {
        dw_main_iteration();
    }
    dw_event_close(&dialog->eve);
    tmp = dialog->result;
    free(dialog);
    return tmp;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 * Returns:
 *       A handle to a box or nullptr on failure.
 */
HWND API dw_box_new(int type, int pad)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID boxNew = env->GetMethodID(clazz, "boxNew", "(II)Landroid/widget/LinearLayout;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, boxNew, type, pad), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 * Returns:
 *       A handle to a groupbox window or nullptr on failure.
 */
HWND API dw_groupbox_new(int type, int pad, const char *title)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = title ? env->NewStringUTF(title) : nullptr;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID groupBoxNew = env->GetMethodID(clazz, "groupBoxNew",
                                                 "(IILjava/lang/String;)Landroid/widget/LinearLayout;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, groupBoxNew, type, pad, jstr), _DW_REFERENCE_WEAK);
        if(jstr)
            env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 * Returns:
 *       A handle to a scrollbox or nullptr on failure.
 */
HWND API dw_scrollbox_new(int type, int pad)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID scrollBoxNew = env->GetMethodID(clazz, "scrollBoxNew",
                                            "(II)Landroid/widget/ScrollView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, scrollBoxNew, type, pad), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Returns the position of the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 * Returns:
 *       The vertical or horizontal position in the scrollbox.
 */
int API dw_scrollbox_get_pos(HWND handle, int orient)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID scrollBoxGetPos = env->GetMethodID(clazz, "scrollBoxGetPos",
                                                     "(Landroid/widget/ScrollView;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, scrollBoxGetPos, handle, orient);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
    }
    return retval;
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 * Returns:
 *       The vertical or horizontal range of the scrollbox.
 */
int API dw_scrollbox_get_range(HWND handle, int orient)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID scrollBoxGetRange = env->GetMethodID(clazz, "scrollBoxGetRange",
                                                       "(Landroid/widget/ScrollView;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, scrollBoxGetRange, handle, orient);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
    }
    return retval;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, const char *funcname)
{
    JNIEnv *env;

    if(box && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID boxPack = env->GetMethodID(clazz, "boxPack", "(Landroid/view/View;Landroid/view/View;IIIIII)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, boxPack, box, item, index, width, height, hsize, vsize, pad);
        _dw_jni_check_exception(env);
    }
}

/*
 * Remove windows (widgets) from the box they are packed into.
 * Parameters:
 *       handle: Window handle of the packed item to be removed.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_box_unpack(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID boxUnpack = env->GetMethodID(clazz, "boxUnpack", "(Landroid/view/View;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, boxUnpack, handle);
        _dw_jni_check_exception(env);
        return DW_ERROR_NONE;
    }
    return DW_ERROR_GENERAL;
}

/*
 * Remove windows (widgets) from a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be removed from.
 *       index: 0 based index of packed items.
 * Returns:
 *       Handle to the removed item on success, 0 on failure or padding.
 */
HWND API dw_box_unpack_at_index(HWND box, int index)
{
    JNIEnv *env;
    HWND retval = nullptr;

    if(box && (env = (JNIEnv *)pthread_getspecific(_dw_env_key))) {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID boxUnpackAtIndex = env->GetMethodID(clazz, "boxUnpackAtIndex",
                                                      "(Landroid/widget/LinearLayout;I)Landroid/view/View;");
        // Call the method on the object
        retval = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, boxUnpackAtIndex, box, index), _DW_REFERENCE_WEAK);
    }
    return retval;
}

/*
 * Pack windows (widgets) into a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       index: 0 based index of packed items.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_at_index(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, index, width, height, hsize, vsize, pad, "dw_box_pack_at_index()");
}

/*
 * Pack windows (widgets) into a box from the start (or top).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    /* 65536 is the table limit on GTK...
     * seems like a high enough value we will never hit it here either.
     */
    _dw_box_pack(box, item, -1, width, height, hsize, vsize, pad, "dw_box_pack_start()");
}

/*
 * Pack windows (widgets) into a box from the end (or bottom).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, 0, width, height, hsize, vsize, pad, "dw_box_pack_end()");
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a button window or nullptr on failure.
 */
HWND API dw_button_new(const char *text, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID buttonNew = env->GetMethodID(clazz, "buttonNew",
                                               "(Ljava/lang/String;I)Landroid/widget/Button;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, buttonNew, jstr, (int)cid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

HWND _dw_entryfield_new(const char *text, ULONG cid, int password)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID entryfieldNew = env->GetMethodID(clazz, "entryfieldNew",
                                                   "(Ljava/lang/String;II)Landroid/widget/EditText;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, entryfieldNew, jstr, (int)cid, password), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}
/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to an entryfield window or nullptr on failure.
 */
HWND API dw_entryfield_new(const char *text, ULONG cid)
{
    return _dw_entryfield_new(text, cid, FALSE);
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to an entryfield password window or nullptr on failure.
 */
HWND API dw_entryfield_password_new(const char *text, ULONG cid)
{
    return _dw_entryfield_new(text, cid, TRUE);
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID entryfieldSetLimit = env->GetMethodID(clazz, "entryfieldSetLimit",
                                                        "(Landroid/widget/EditText;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, entryfieldSetLimit, handle, (jlong)limit);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 * Returns:
 *       A handle to a bitmap button window or nullptr on failure.
 */
HWND API dw_bitmapbutton_new(const char *text, ULONG resid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID bitmapButtonNew = env->GetMethodID(clazz, "bitmapButtonNew",
                                                     "(Ljava/lang/String;I)Landroid/widget/ImageButton;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, bitmapButtonNew, jstr, (int)resid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a bitmap button window or nullptr on failure.
 */
HWND API dw_bitmapbutton_new_from_file(const char *text, unsigned long cid, const char *filename)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        jstring path = env->NewStringUTF(filename);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID bitmapButtonNewFromFile = env->GetMethodID(clazz, "bitmapButtonNewFromFile",
                                                             "(Ljava/lang/String;ILjava/lang/String;)Landroid/widget/ImageButton;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, bitmapButtonNewFromFile, jstr, (int)cid, path), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        env->DeleteLocalRef(path);
        return result;
    }
    return nullptr;
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: The contents of the image
 *            (BMP or ICO on OS/2 or Windows, XPM on Unix)
 *       len: length of str
 * Returns:
 *       A handle to a bitmap button window or nullptr on failure.
 */
HWND API dw_bitmapbutton_new_from_data(const char *text, unsigned long cid, const char *data, int len)
{
    JNIEnv *env;

    if(data && len > 0 && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // Construct a byte array
        jbyteArray bytearray = env->NewByteArray(len);
        env->SetByteArrayRegion(bytearray, 0, len, reinterpret_cast<const jbyte *>(data));
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID bitmapButtonNewFromData = env->GetMethodID(clazz, "bitmapButtonNewFromData",
                                                             "(Ljava/lang/String;I[BI)Landroid/widget/ImageButton;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, bitmapButtonNewFromData, jstr, (int)cid, bytearray, len), _DW_REFERENCE_WEAK);
        // TODO: Clean up after the array now that we are finished
        //env->ReleaseByteArrayElements(bytearray, (jbyte *) data, 0);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a spinbutton window or nullptr on failure.
 */
HWND API dw_spinbutton_new(const char *text, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID spinButtonNew = env->GetMethodID(clazz, "spinButtonNew",
                                                   "(Ljava/lang/String;I)Lorg/dbsoft/dwindows/DWSpinButton;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, spinButtonNew, jstr, (int)cid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID spinButtonSetPos = env->GetMethodID(clazz, "spinButtonSetPos",
                                                      "(Lorg/dbsoft/dwindows/DWSpinButton;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, spinButtonSetPos, handle, (jlong)position);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          upper: Upper limit.
 *          lower: Lower limit.
 */
void API dw_spinbutton_set_limits(HWND handle, long upper, long lower)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID spinButtonSetLimits = env->GetMethodID(clazz, "spinButtonSetLimits",
                                                         "(Lorg/dbsoft/dwindows/DWSpinButton;JJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, spinButtonSetLimits, handle, (jlong)upper, (jlong)lower);
        _dw_jni_check_exception(env);
    }
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 * Returns:
 *       Number value displayed in the spinbutton.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
    JNIEnv *env;
    long retval = 0;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID spinButtonGetPos = env->GetMethodID(clazz, "spinButtonGetPos",
                                                      "(Lorg/dbsoft/dwindows/DWSpinButton;)J");
        // Call the method on the object
        retval = env->CallLongMethod(_dw_obj, spinButtonGetPos, handle);
        if(_dw_jni_check_exception(env))
            retval = 0;
    }
    return retval;
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a radio button window or nullptr on failure.
 */
HWND API dw_radiobutton_new(const char *text, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID radioButtonNew = env->GetMethodID(clazz, "radioButtonNew",
                                                    "(Ljava/lang/String;I)Landroid/widget/RadioButton;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, radioButtonNew, jstr, (int)cid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a slider window or nullptr on failure.
 */
HWND API dw_slider_new(int vertical, int increments, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID sliderNew = env->GetMethodID(clazz, "sliderNew",
                                               "(III)Lorg/dbsoft/dwindows/DWSlider;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, sliderNew, vertical, increments, (jint)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 * Returns:
 *       Position of the slider in the set range.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
    JNIEnv *env;
    unsigned int retval = 0;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID percentGetPos = env->GetMethodID(clazz, "percentGetPos",
                                                   "(Landroid/view/View;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, percentGetPos, handle);
        if(_dw_jni_check_exception(env))
            retval = 0;
    }
    return retval;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
    dw_percent_set_pos(handle, position);
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a scrollbar window or nullptr on failure.
 */
HWND API dw_scrollbar_new(int vertical, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID scrollBarNew = env->GetMethodID(clazz, "scrollBarNew",
                                                  "(II)Lorg/dbsoft/dwindows/DWSlider;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, scrollBarNew, vertical, (jint)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 * Returns:
 *       Position of the scrollbar in the set range.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
    return dw_slider_get_pos(handle);
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
    dw_percent_set_pos(handle, position);
}

/*
 * Sets the scrollbar range.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          range: Maximum range value.
 *          visible: Visible area relative to the range.
 */
void API dw_scrollbar_set_range(HWND handle, unsigned int range, unsigned int visible)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID percentSetRange = env->GetMethodID(clazz, "percentSetRange",
                                                     "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, percentSetRange, handle, (jint)range);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a percent bar window or nullptr on failure.
 */
HWND API dw_percent_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID percentNew = env->GetMethodID(clazz, "percentNew",
                                               "(I)Landroid/widget/ProgressBar;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, percentNew, (jint)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID percentSetPos = env->GetMethodID(clazz, "percentSetPos",
                                                   "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, percentSetPos, handle, (jint)position);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a checkbox window or nullptr on failure.
 */
HWND API dw_checkbox_new(const char *text, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID checkboxNew = env->GetMethodID(clazz, "checkboxNew",
                                                 "(Ljava/lang/String;I)Landroid/widget/CheckBox;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, checkboxNew, jstr, (int)cid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 * Returns:
 *       State of checkbox (TRUE or FALSE).
 */
int API dw_checkbox_get(HWND handle)
{
    JNIEnv *env;
    int retval = FALSE;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID checkOrRadioGetChecked = env->GetMethodID(clazz, "checkOrRadioGetChecked",
                                                            "(Landroid/view/View;)Z");
        // Call the method on the object
        retval = env->CallBooleanMethod(_dw_obj, checkOrRadioGetChecked, handle);
        if(_dw_jni_check_exception(env))
            retval = FALSE;
    }
    return retval;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID checkOrRadioSetChecked = env->GetMethodID(clazz, "checkOrRadioSetChecked",
                                                            "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, checkOrRadioSetChecked, handle, value);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 * Returns:
 *       A handle to a listbox window or nullptr on failure.
 */
HWND API dw_listbox_new(ULONG cid, int multi)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listBoxNew = env->GetMethodID(clazz, "listBoxNew",
                                                 "(II)Lorg/dbsoft/dwindows/DWListBox;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, listBoxNew, (int)cid, multi), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, const char *text)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxAppend = env->GetMethodID(clazz, "listOrComboBoxAppend",
                                                          "(Landroid/view/View;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxAppend, handle, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Inserts the specified text into the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be inserted into.
 *          text: Text to insert into listbox.
 *          pos: 0-based position to insert text
 */
void API dw_listbox_insert(HWND handle, const char *text, int pos)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxInsert = env->GetMethodID(clazz, "listOrComboBoxInsert",
                                                          "(Landroid/view/View;Ljava/lang/String;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxInsert, handle, jstr, pos);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Appends the specified text items to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text strings to append into listbox.
 *          count: Number of text strings to append
 */
void API dw_listbox_list_append(HWND handle, char **text, int count)
{
    int x;

    /* TODO: this would be more efficient passing in an array */
    for(x=0;x<count;x++)
        dw_listbox_append(handle, text[x]);
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxClear = env->GetMethodID(clazz, "listOrComboBoxClear",
                                                          "(Landroid/view/View;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxClear, handle);
        _dw_jni_check_exception(env);
    }
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be counted.
 * Returns:
 *       The number of items in the listbox.
 */
int API dw_listbox_count(HWND handle)
{
    JNIEnv *env;
    int retval = 0;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxCount = env->GetMethodID(clazz, "listOrComboBoxCount",
                                                         "(Landroid/view/View;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, listOrComboBoxCount, handle);
        if(_dw_jni_check_exception(env))
            retval = 0;
    }
    return retval;
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void API dw_listbox_set_top(HWND handle, int top)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listBoxSetTop = env->GetMethodID(clazz, "listBoxSetTop",
                                                         "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listBoxSetTop, handle, top);
        _dw_jni_check_exception(env);
    }
}

/*
 * Copies the given index item's text into buffer.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 *          length: Length of the buffer (including nullptr).
 */
void API dw_listbox_get_text(HWND handle, unsigned int index, char *buffer, unsigned int length)
{
    JNIEnv *env;

    if(buffer && length > 0 && handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxGetText = env->GetMethodID(clazz, "listOrComboBoxGetText",
                                                           "(Landroid/view/View;I)Ljava/lang/String;");
        // Call the method on the object
        jstring result = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, listOrComboBoxGetText, handle, index), _DW_REFERENCE_NONE);
        // Get the UTF8 string result
        if(result)
        {
            const char *utf8 = env->GetStringUTFChars(result, nullptr);

            if(utf8)
            {
                strncpy(buffer, utf8, length);
                env->ReleaseStringUTFChars(result, utf8);
            }
        }
    }
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
void API dw_listbox_set_text(HWND handle, unsigned int index, const char *buffer)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(buffer);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxSetText = env->GetMethodID(clazz, "listOrComboBoxSetText",
                                                          "(Landroid/view/View;ILjava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxSetText, handle, index, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 * Returns:
 *       The selected item index or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_listbox_selected(HWND handle)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxGetSelected = env->GetMethodID(clazz, "listOrComboBoxGetSelected",
                                                           "(Landroid/view/View;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, listOrComboBoxGetSelected, handle);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
    }
    return retval;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 * Returns:
 *       The next selected item or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listBoxSelectedMulti = env->GetMethodID(clazz, "listBoxSelectedMulti",
                                                          "(Landroid/view/View;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, listBoxSelectedMulti, handle, where);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
    }
    return retval;
}

/*
 * Sets the selection state of a given index.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 *          state: TRUE if selected FALSE if unselected.
 */
void API dw_listbox_select(HWND handle, int index, int state)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxSelect = env->GetMethodID(clazz, "listOrComboBoxSelect",
                                                           "(Landroid/view/View;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxSelect, handle, index, state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID listOrComboBoxDelete = env->GetMethodID(clazz, "listOrComboBoxDelete",
                                                          "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, listOrComboBoxDelete, handle, index);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a combobox window or nullptr on failure.
 */
HWND API dw_combobox_new(const char *text, ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID comboBoxNew = env->GetMethodID(clazz, "comboBoxNew",
                                                 "(Ljava/lang/String;I)Lorg/dbsoft/dwindows/DWComboBox;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, comboBoxNew, jstr, (int)cid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a MLE window or nullptr on failure.
 */
HWND API dw_mle_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleNew = env->GetMethodID(clazz, "mleNew",
                                            "(I)Landroid/widget/EditText;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, mleNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 * Returns:
 *       Current position in the buffer.
 */
unsigned int API dw_mle_import(HWND handle, const char *buffer, int startpoint)
{
    JNIEnv *env;
    int retval = 0;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(buffer);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleImport = env->GetMethodID(clazz, "mleImport",
                                               "(Landroid/widget/EditText;Ljava/lang/String;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, mleImport, handle, jstr, startpoint);
        if(_dw_jni_check_exception(env))
            retval = 0;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
void API dw_mle_export(HWND handle, char *buffer, int startpoint, int length)
{
    if(buffer && length > 0) {
        char *text = dw_window_get_text(handle);

        if (text) {
            int len = strlen(text);

            if (startpoint < len)
                strncpy(buffer, &text[startpoint], length);
            else
                buffer[0] = '\0';
            free(text);
        }
    }
}

/*
 * Obtains information about an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          bytes: A pointer to a variable to return the total bytes.
 *          lines: A pointer to a variable to return the number of lines.
 */
void API dw_mle_get_size(HWND handle, unsigned long *bytes, unsigned long *lines)
{
    char *text = dw_window_get_text(handle);

    if(bytes) {
        if(text) {
            *bytes = strlen(text);
        } else {
            *bytes = 0;
        }
    }
    if(lines) {
        if(text)
        {
            int count = 0;
            char *tmp = text;

            while((tmp = strchr(tmp, '\n'))) {
                count++;
            }
            *lines = count;
        } else {
            *lines = 0;
        }
    }
    if(text)
        free(text);
}

/*
 * Deletes text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be deleted from.
 *          startpoint: Point to start deleting text.
 *          length: Amount of text to be deleted.
 */
void API dw_mle_delete(HWND handle, int startpoint, int length)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleDelete = env->GetMethodID(clazz, "mleDelete",
                                               "(Landroid/widget/EditText;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleDelete, handle, startpoint, length);
        _dw_jni_check_exception(env);
    }
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleClear = env->GetMethodID(clazz, "mleClear",
                                              "(Landroid/widget/EditText;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleClear, handle);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSetVisible = env->GetMethodID(clazz, "mleSetVisible",
                                                    "(Landroid/widget/EditText;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleSetVisible, handle, line);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSetEditable = env->GetMethodID(clazz, "mleSetEditable",
                                                    "(Landroid/widget/EditText;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleSetEditable, handle, state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSetWordWrap = env->GetMethodID(clazz, "mleSetWordWrap",
                                                    "(Landroid/widget/EditText;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleSetWordWrap, handle, state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void API dw_mle_set_cursor(HWND handle, int point)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSetCursor = env->GetMethodID(clazz, "mleSetCursor",
                                                    "(Landroid/widget/EditText;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleSetCursor, handle, point);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
void API dw_mle_set_auto_complete(HWND handle, int state)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSetAutoComplete = env->GetMethodID(clazz, "mleSetAutoComplete",
                                                  "(Landroid/widget/EditText;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, mleSetAutoComplete, handle, state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 * Returns:
 *       Position in buffer or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_mle_search(HWND handle, const char *text, int point, unsigned long flags)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleSearch = env->GetMethodID(clazz, "mleSearch",
                                               "(Landroid/widget/EditText;Ljava/lang/String;II)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, mleSearch, handle, jstr, point, (jint)flags);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
    /* Don't think this is necessary on Android */
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
    /* Don't think this is necessary on Android */
}

HWND _dw_text_new(const char *text, ULONG cid, int status)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID textNew = env->GetMethodID(clazz, "textNew",
                                             "(Ljava/lang/String;II)Landroid/widget/TextView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, textNew, jstr, (int)cid, status), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a status text window or nullptr on failure.
 */
HWND API dw_status_text_new(const char *text, ULONG cid)
{
    return _dw_text_new(text, cid, TRUE);
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a text window or nullptr on failure.
 */
HWND API dw_text_new(const char *text, ULONG cid)
{
    return _dw_text_new(text, cid, FALSE);
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or nullptr on failure.
 */
HWND API dw_render_new(unsigned long cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID renderNew = env->GetMethodID(clazz, "renderNew",
                                               "(I)Lorg/dbsoft/dwindows/DWRender;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, renderNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
void API dw_render_redraw(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID renderRedraw = env->GetMethodID(clazz, "renderRedraw",
                                                  "(Lorg/dbsoft/dwindows/DWRender;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, renderRedraw, handle, (jint)_dw_render_safe_mode);
        _dw_jni_check_exception(env);
    }
}

/* Internal function to create backing bitmap */
void _dw_render_create_bitmap(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID renderCreateBitmap = env->GetMethodID(clazz, "renderCreateBitmap",
                                                  "(Lorg/dbsoft/dwindows/DWRender;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, renderCreateBitmap, handle);
        _dw_jni_check_exception(env);
    }
}

/* Internal function to trigger a redraw using the backing bitmap */
void _dw_render_finish_draw(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID renderFinishDraw = env->GetMethodID(clazz, "renderFinishDraw",
                                                        "(Lorg/dbsoft/dwindows/DWRender;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, renderFinishDraw, handle);
        _dw_jni_check_exception(env);
    }
}


/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
    pthread_setspecific(_dw_fgcolor_key, (void *)value);
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
    pthread_setspecific(_dw_bgcolor_key, (void *)value);
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
    JNIEnv *env;
    unsigned long newcolor = value;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        unsigned long _value = _dw_get_color(value);
        int r, g, b;
        jint ac;

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID colorChoose = env->GetMethodID(clazz, "colorChoose",
                                                 "(IIIII)I");
        // Call the method on the object
        ac = env->CallIntMethod(_dw_obj, colorChoose, (jint)value, 0,
                   (jint)DW_RED_VALUE(_value), (jint)DW_GREEN_VALUE(_value), (jint)DW_BLUE_VALUE(_value));
        if(_dw_jni_check_exception(env))
            return value;
        // Convert from Android Color to RGB back to Dynamic Windows
        b = ac & 0xff;
        g = (ac >> 8) & 0xff;
        r = (ac >> 16) & 0xff;
        newcolor = DW_RGB(r, g, b);
    }
    return newcolor;
}

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_draw_point(HWND handle, HPIXMAP pixmap, int x, int y)
{
    JNIEnv *env;

    if((handle || pixmap) && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
        jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID drawPoint = env->GetMethodID(clazz, "drawPoint",
                                               "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIJJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, drawPoint, handle, pixmap ? pixmap->bitmap : nullptr, x, y, fgcolor, bgcolor);
        _dw_jni_check_exception(env);
    }
}

/* Draw a line on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x1: First X coordinate.
 *       y1: First Y coordinate.
 *       x2: Second X coordinate.
 *       y2: Second Y coordinate.
 */
void API dw_draw_line(HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2)
{
    JNIEnv *env;

    if((handle || pixmap) && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
        jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID drawLine = env->GetMethodID(clazz, "drawLine",
                                              "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIIIJJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, drawLine, handle, pixmap ? pixmap->bitmap : nullptr, x1, y1, x2, y2, fgcolor, bgcolor);
        _dw_jni_check_exception(env);
    }
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 *       text: Text to be displayed.
 */
void API dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, const char *text)
{
    JNIEnv *env;

    if((handle || pixmap) && text && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
        jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

        // Construct the string
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID drawLine = env->GetMethodID(clazz, "drawText",
                                              "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IILjava/lang/String;Landroid/graphics/Typeface;ILandroid/view/View;JJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, drawLine, handle, pixmap ? pixmap->bitmap : nullptr, x, y, jstr,
                            pixmap ? pixmap->typeface : nullptr, pixmap ? pixmap->fontsize : 0,
                            pixmap ? pixmap->handle : nullptr, fgcolor, bgcolor);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height: Pointer to a variable to be filled in with the height.
 */
void API dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
{
    JNIEnv *env;

    if((handle || pixmap) && text && (width || height) && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct the string
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID fontTextExtentsGet = env->GetMethodID(clazz, "fontTextExtentsGet",
                                                        "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;Ljava/lang/String;Landroid/graphics/Typeface;ILandroid/view/View;)J");
        // Call the method on the object
        jlong dimensions = env->CallLongMethod(_dw_obj, fontTextExtentsGet, handle, pixmap ? pixmap->bitmap : nullptr, jstr,
                            pixmap ? pixmap->typeface : nullptr, pixmap ? pixmap->fontsize : 0, pixmap ? pixmap->handle : nullptr);
        if(_dw_jni_check_exception(env))
            dimensions = 0;

        if(width)
            *width = dimensions & 0xFFFF;
        if(height)
            *height = (dimensions >> 32) & 0xFFFF;
        env->DeleteLocalRef(jstr);
    }
}

/* Draw a polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       npoints: Number of points passed in.
 *       x: Pointer to array of X coordinates.
 *       y: Pointer to array of Y coordinates.
 */
void API dw_draw_polygon(HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
{
    JNIEnv *env;

    if((handle || pixmap) && npoints > 0 && x && y && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jintArray jx = env->NewIntArray(npoints);
        jintArray jy = env->NewIntArray(npoints);

        if(jx && jy)
        {
            jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
            jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

            // Construct the integer arrays
            env->SetIntArrayRegion(jx, 0, npoints, x);
            env->SetIntArrayRegion(jy, 0, npoints, y);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID drawPolygon = env->GetMethodID(clazz, "drawPolygon",
                                                     "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;II[I[IJJ)V");
            // Call the method on the object
            env->CallVoidMethod(_dw_obj, drawPolygon, handle, pixmap ? pixmap->bitmap : nullptr, flags, npoints, jx, jy, fgcolor, bgcolor);
            _dw_jni_check_exception(env);
        }
    }
}

/* Draw a rectangle on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the box or DW_DRAW_DEFAULT (0).
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void API dw_draw_rect(HWND handle, HPIXMAP pixmap, int flags, int x, int y, int width, int height)
{
    JNIEnv *env;

    if((handle || pixmap) && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
        jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID drawLine = env->GetMethodID(clazz, "drawRect",
                                              "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIIIJJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, drawLine, handle, pixmap ? pixmap->bitmap : nullptr, x, y, width, height, fgcolor, bgcolor);
        _dw_jni_check_exception(env);
    }
}

/* Draw an arc on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the arc or DW_DRAW_DEFAULT (0).
 *              DW_DRAW_FULL will draw a complete circle/elipse.
 *       xorigin: X coordinate of center of arc.
 *       yorigin: Y coordinate of center of arc.
 *       x1: X coordinate of first segment of arc.
 *       y1: Y coordinate of first segment of arc.
 *       x2: X coordinate of second segment of arc.
 *       y2: Y coordinate of second segment of arc.
 */
void API dw_draw_arc(HWND handle, HPIXMAP pixmap, int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2)
{
    JNIEnv *env;

    if((handle || pixmap) && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong fgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_fgcolor_key));
        jlong bgcolor = _dw_get_color((unsigned long)pthread_getspecific(_dw_bgcolor_key));

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID drawLine = env->GetMethodID(clazz, "drawArc",
                                              "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIIIIIIJJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, drawLine, handle, pixmap ? pixmap->bitmap : nullptr, flags, xorigin, yorigin, x1, y1, x2, y2, fgcolor, bgcolor);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       A handle to a tree window or nullptr on failure.
 */
HWND API dw_tree_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeNew = env->GetMethodID(clazz, "treeNew",
                                             "(I)Lorg/dbsoft/dwindows/DWTree;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, treeNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Inserts an item into a tree window (widget) after another item.
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          item: Handle to the item to be positioned after.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 * Returns:
 *       A handle to a tree item or nullptr on failure.
 */
HTREEITEM API dw_tree_insert_after(HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    JNIEnv *env;

    if(handle && title && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct the string
        jstring jstr = env->NewStringUTF(title);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeInsertAfter = env->GetMethodID(clazz, "treeInsertAfter",
                                                     "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;Ljava/lang/String;Landroid/graphics/drawable/Drawable;Lorg/dbsoft/dwindows/DWTreeItem;J)Lorg/dbsoft/dwindows/DWTreeItem;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, treeInsertAfter, handle, item,
                                                                         jstr, icon, parent, (jlong)itemdata), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Inserts an item into a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 * Returns:
 *       A handle to a tree item or nullptr on failure.
 */
HTREEITEM API dw_tree_insert(HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    return dw_tree_insert_after(handle, nullptr, title, icon, parent, itemdata);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A malloc()ed buffer of item text to be dw_free()ed or nullptr on error.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        const char *utf8 = nullptr;
        char *retval = nullptr;

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeGetTitle = env->GetMethodID(clazz, "treeGetTitle",
                                                   "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;)Ljava/lang/String;");
        // Call the method on the object
        jstring result = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, treeGetTitle, handle, item), _DW_REFERENCE_NONE);
        // Get the UTF8 string result
        if(result)
            utf8 = env->GetStringUTFChars(result, nullptr);
        if(utf8)
        {
            retval = strdup(utf8);
            env->ReleaseStringUTFChars(result, utf8);
        }
    }
    return nullptr;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A handle to a tree item or nullptr on failure.
 */
HTREEITEM API dw_tree_get_parent(HWND handle, HTREEITEM item)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeGetParent = env->GetMethodID(clazz, "treeGetParent",
                                                  "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;)Lorg/dbsoft/dwindows/DWTreeItem;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, treeGetParent, handle, item), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void API dw_tree_item_change(HWND handle, HTREEITEM item, const char *title, HICN icon)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct the string
        jstring jstr = title ? env->NewStringUTF(title) : nullptr;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemChange = env->GetMethodID(clazz, "treeItemChange",
                                                    "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;Ljava/lang/String;Landroid/graphics/drawable/Drawable;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeItemChange, handle, item, jstr, icon);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
void API dw_tree_item_set_data(HWND handle, HTREEITEM item, void *itemdata)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong data = (jlong)itemdata;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemSetData = env->GetMethodID(clazz, "treeItemSetData",
                                                     "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeItemSetData, handle, item, data);
    }
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A pointer to tree item data or nullptr on failure.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
    JNIEnv *env;
    void *retval = nullptr;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemGetData = env->GetMethodID(clazz, "treeItemGetData",
                                                     "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;)J");
        // Call the method on the object
        retval = (void *)env->CallLongMethod(_dw_obj, treeItemGetData, handle, item);
    }
    return retval;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemSelect = env->GetMethodID(clazz, "treeItemSelect",
                                                    "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeItemSelect, handle, item);
    }
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeClear = env->GetMethodID(clazz, "treeClear",
                                               "(Lorg/dbsoft/dwindows/DWTree;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeClear, handle);
    }
}

/* Internal function to set the expanded state */
void _dw_tree_item_set_expanded(HWND handle, HTREEITEM item, jint state)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemExpand = env->GetMethodID(clazz, "treeItemExpand",
                                                    "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeItemExpand, handle, item, state);
    }
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
    _dw_tree_item_set_expanded(handle, item, TRUE);
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
    _dw_tree_item_set_expanded(handle, item, FALSE);
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
    JNIEnv *env;

    if(handle && item && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID treeItemDelete = env->GetMethodID(clazz, "treeItemDelete",
                                                    "(Lorg/dbsoft/dwindows/DWTree;Lorg/dbsoft/dwindows/DWTreeItem;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, treeItemDelete, handle, item);
    }
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       A handle to a container window or nullptr on failure.
 */
HWND API dw_container_new(ULONG cid, int multi)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerNew = env->GetMethodID(clazz, "containerNew",
                                                  "(II)Landroid/widget/ListView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, containerNew, (int)cid, multi), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Sets up the container columns.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 *          separator: The column number that contains the main separator.
 *                     (this item may only be used in OS/2)
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator)
{
    int retval = DW_ERROR_GENERAL;

    if(handle && flags && titles && count > 0)
    {
        int z;

        for(z=0;z<count;z++)
        {
            JNIEnv *env;

            if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
            {
                retval = DW_ERROR_NONE;

                // Generate a string
                jstring jstr = env->NewStringUTF(titles[z]);
                // First get the class that contains the method you need to call
                jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
                // Get the method that you want to call
                jmethodID containerAddColumn = env->GetMethodID(clazz, "containerAddColumn",
                                                          "(Landroid/widget/ListView;Ljava/lang/String;I)V");
                // Call the method on the object
                env->CallVoidMethod(_dw_obj, containerAddColumn, handle, jstr, (int)flags[z]);
                if(_dw_jni_check_exception(env))
                    retval = DW_ERROR_GENERAL;
                env->DeleteLocalRef(jstr);
            }
        }
    }
    return retval;
}

/*
 * Configures the main filesystem column title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, const char *title)
{
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count)
{
    unsigned long fsflags[2] = { DW_CFA_BITMAPORICON, DW_CFA_STRING };
    char *fstitles[2] = { (char *)"Icon", (char *)"Filename" };
    int retval = dw_container_setup(handle, fsflags, fstitles, 2, 0);

    if(retval == DW_ERROR_NONE)
        retval = dw_container_setup(handle, flags, titles, count, 0);
    return retval;
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 * Returns:
 *       Handle to container items allocated or nullptr on error.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerAlloc = env->GetMethodID(clazz, "containerAlloc",
                                                    "(Landroid/widget/ListView;I)Landroid/widget/ListView;");
        // Call the method on the object
        return (void *)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, containerAlloc, handle, rowcount), _DW_REFERENCE_WEAK);
    }
    return nullptr;
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_container_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
    int rowstart = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_rowstart"));

    dw_container_change_item(handle, column, row + rowstart, data);
}

/* Internal version that doesn't update the list immediately */
void API _dw_container_change_item(HWND handle, int column, int row, void *data)
{
    JNIEnv *env;

    if(handle && data && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        int columntype = dw_container_get_column_type(handle, column);

        if((columntype & DW_CFA_BITMAPORICON))
        {
            jobject icon = *((jobject *)data);

            if(icon)
            {
                // First get the class that contains the method you need to call
                jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
                // Get the method that you want to call
                jmethodID containerChangeItem = env->GetMethodID(clazz, "containerChangeItemIcon",
                                                                 "(Landroid/widget/ListView;IILandroid/graphics/drawable/Drawable;)V");
                // Call the method on the object
                env->CallVoidMethod(_dw_obj, containerChangeItem, handle, column, row, icon);
                _dw_jni_check_exception(env);
            }
        }
        else if((columntype & DW_CFA_STRING))
        {
            const char *tmp = *((const char **)data);

            if(tmp)
            {
                jstring jstr = env->NewStringUTF(tmp);
                // First get the class that contains the method you need to call
                jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
                // Get the method that you want to call
                jmethodID containerChangeItem = env->GetMethodID(clazz, "containerChangeItemString",
                                                                 "(Landroid/widget/ListView;IILjava/lang/String;)V");
                // Call the method on the object
                env->CallVoidMethod(_dw_obj, containerChangeItem, handle, column, row, jstr);
                _dw_jni_check_exception(env);
                env->DeleteLocalRef(jstr);
            }
        }
        else if((columntype & DW_CFA_ULONG))
        {
            ULONG num = *((ULONG *)data);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerChangeItem = env->GetMethodID(clazz, "containerChangeItemInt",
                                                             "(Landroid/widget/ListView;IIJ)V");
            // Call the method on the object
            env->CallVoidMethod(_dw_obj, containerChangeItem, handle, column, row, (jlong)num);
            _dw_jni_check_exception(env);
        }
        else if((columntype & DW_CFA_DATE))
        {
            CDATE cdate = *((CDATE *)data);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerChangeItem = env->GetMethodID(clazz, "containerChangeItemDate",
                                                             "(Landroid/widget/ListView;IIIII)V");
            // Call the method on the object
            env->CallVoidMethod(_dw_obj, containerChangeItem, handle, column, row,
                                (jint)cdate.year, (jint)cdate.month, (jint)cdate.day);
            _dw_jni_check_exception(env);
        }
        else if((columntype & DW_CFA_TIME))
        {
            CTIME ctime = *((CTIME *)data);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerChangeItem = env->GetMethodID(clazz, "containerChangeItemTime",
                                                             "(Landroid/widget/ListView;IIIII)V");
            // Call the method on the object
            env->CallVoidMethod(_dw_obj, containerChangeItem, handle, column, row,
                                (jint)ctime.hours, (jint)ctime.minutes, (jint)ctime.seconds);
            _dw_jni_check_exception(env);
        }
    }
}

/* Notify that the data changed, causing the container to refresh */
void _dw_container_refresh(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerRefresh = env->GetMethodID(clazz, "containerRefresh",
                                                      "(Landroid/widget/ListView;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerRefresh, handle);
        _dw_jni_check_exception(env);
    }
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_container_change_item(HWND handle, int column, int row, void *data)
{
    _dw_container_change_item(handle, column, row, data);
    _dw_container_refresh(handle);
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_item(HWND handle, int column, int row, void *data)
{
    _dw_container_change_item(handle, column + 2, row, data);
    _dw_container_refresh(handle);
}

/*
 * Changes an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_file(HWND handle, int row, const char *filename, HICN icon)
{
    _dw_container_change_item(handle, 0, row, (void *)&icon);
    _dw_container_change_item(handle, 1, row, (void *)&filename);
    _dw_container_refresh(handle);
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_set_file(HWND handle, void *pointer, int row, const char *filename, HICN icon)
{
    dw_container_set_item(handle, pointer, 0, row, (void *)&icon);
    dw_container_set_item(handle, pointer, 1, row, (void *)&filename);
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
    dw_container_set_item(handle, pointer, column + 2, row, data);
}

/*
 * Sets the data of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_set_row_data(void *pointer, int row, void *data)
{
    HWND handle = (HWND)pointer;

    if(handle)
    {
        int rowstart = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_rowstart"));

        dw_container_change_row_data(handle, row + rowstart, data);
    }
}

/*
 * Changes the data of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_change_row_data(HWND handle, int row, void *data)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerChangeRowData = env->GetMethodID(clazz, "containerChangeRowData",
                                                            "(Landroid/widget/ListView;IJ)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerChangeRowData, handle, row, (jlong)data);
        _dw_jni_check_exception(env);
    }
}

/*
 * Gets column type for a container column.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 * Returns:
 *       Constant identifying the the column type.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
    JNIEnv *env;
    int retval = 0;
    int mask = (DW_CFA_BITMAPORICON | DW_CFA_STRING | DW_CFA_ULONG | DW_CFA_TIME | DW_CFA_DATE);

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerGetColumnType = env->GetMethodID(clazz, "containerGetColumnType",
                                                            "(Landroid/widget/ListView;I)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, containerGetColumnType, handle, column);
        if(_dw_jni_check_exception(env))
            retval = 0;
    }
    return (retval & mask);
}

/*
 * Gets column type for a filesystem container column.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 * Returns:
 *       Constant identifying the the column type.
 */
int API dw_filesystem_get_column_type(HWND handle, int column)
{
    return dw_container_get_column_type(handle, column + 2);
}

/*
 * Sets the alternating row colors for container window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          oddcolor: Odd row background color in DW_RGB format or a default color index.
 *          evencolor: Even row background color in DW_RGB format or a default color index.
 *                    DW_RGB_TRANSPARENT will disable coloring rows.
 *                    DW_CLR_DEFAULT will use the system default alternating row colors.
 */
void API dw_container_set_stripe(HWND handle, unsigned long oddcolor, unsigned long evencolor)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jlong odd = _dw_get_color(oddcolor);
        jlong even = _dw_get_color(evencolor);

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerSetStripe = env->GetMethodID(clazz, "containerSetStripe",
                                                        "(Landroid/widget/ListView;JJ)V");
        if(oddcolor == DW_RGB_TRANSPARENT)
            odd = -1;
        else if(oddcolor == DW_CLR_DEFAULT)
            odd = -2;
        if(evencolor == DW_RGB_TRANSPARENT)
            even = -1;
        else if(evencolor == DW_CLR_DEFAULT)
            even = -2;

        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerSetStripe, handle, odd, even);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the width of a column in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          column: Zero based column of width being set.
 *          width: Width of column in pixels.
 */
void API dw_container_set_column_width(HWND handle, int column, int width)
{
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_set_row_title(void *pointer, int row, const char *title)
{
    HWND handle = (HWND)pointer;

    if(handle)
    {
        int rowstart = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_rowstart"));

        dw_container_change_row_title(handle, row + rowstart, title);
    }
}


/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_change_row_title(HWND handle, int row, const char *title)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Generate a string
        jstring jstr = title ? env->NewStringUTF(title) : nullptr;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerChangeRowTitle = env->GetMethodID(clazz, "containerChangeRowTitle",
                                                             "(Landroid/widget/ListView;ILjava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerChangeRowTitle, handle, row, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          rowcount: The number of rows to be inserted.
 */
void API dw_container_insert(HWND handle, void *pointer, int rowcount)
{
    JNIEnv *env;

    if(pointer && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        env->DeleteWeakGlobalRef((jobject)pointer);
    dw_window_set_data(handle, "_dw_rowstart", NULL);
    _dw_container_refresh(handle);
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerClear = env->GetMethodID(clazz, "containerClear",
                                                    "(Landroid/widget/ListView;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerClear, handle);
        _dw_jni_check_exception(env);
        _dw_container_refresh(handle);
    }
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerDelete = env->GetMethodID(clazz, "containerDelete",
                                                     "(Landroid/widget/ListView;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerDelete, handle, rowcount);
        _dw_jni_check_exception(env);
        _dw_container_refresh(handle);
    }
}

/*
 * Scrolls container up or down.
 * Parameters:
 *       handle: Handle to the window (widget) to be scrolled.
 *       direction: DW_SCROLL_UP, DW_SCROLL_DOWN, DW_SCROLL_TOP or
 *                  DW_SCROLL_BOTTOM. (rows is ignored for last two)
 *       rows: The number of rows to be scrolled.
 */
void API dw_container_scroll(HWND handle, int direction, long rows)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerScroll = env->GetMethodID(clazz, "containerScroll",
                                                     "(Landroid/widget/ListView;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerScroll, handle, direction, (jint)rows);
        _dw_jni_check_exception(env);
    }
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 * Returns:
 *       Pointer to data associated with first entry or nullptr on error.
 */
char * API dw_container_query_start(HWND handle, unsigned long flags)
{
    JNIEnv *env;
    char *retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        if(flags & DW_CR_RETDATA) {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerGetDataStart = env->GetMethodID(clazz, "containerGetDataStart",
                                                               "(Landroid/widget/ListView;I)J");
            // Call the method on the object
            jlong data = env->CallLongMethod(_dw_obj, containerGetDataStart, handle, (jint)flags);
            if(!_dw_jni_check_exception(env))
                retval = (char *)data;
        } else {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerGetTitleStart = env->GetMethodID(clazz, "containerGetTitleStart",
                                                    "(Landroid/widget/ListView;I)Ljava/lang/String;");
            // Call the method on the object
            jstring jstr = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, containerGetTitleStart, handle, (jint)flags), _DW_REFERENCE_NONE);
            if(jstr)
            {
                const char *str = env->GetStringUTFChars(jstr, nullptr);

                if(str)
                {
                    retval = strdup(str);
                    env->ReleaseStringUTFChars(jstr, str);
                }
            }
        }
    }
    return retval;
}

/*
 * Continues an existing query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 * Returns:
 *       Pointer to data associated with next entry or nullptr on error or completion.
 */
char * API dw_container_query_next(HWND handle, unsigned long flags)
{
    JNIEnv *env;
    char *retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        if(flags & DW_CR_RETDATA) {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerGetDataStart = env->GetMethodID(clazz, "containerGetDataNext",
                                                               "(Landroid/widget/ListView;I)J");
            // Call the method on the object
            jlong data = env->CallLongMethod(_dw_obj, containerGetDataStart, handle, (jint)flags);
            if(!_dw_jni_check_exception(env))
                retval = (char *)data;
        } else {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID containerGetTitleNext = env->GetMethodID(clazz, "containerGetTitleNext",
                                                               "(Landroid/widget/ListView;I)Ljava/lang/String;");
            // Call the method on the object
            jstring jstr = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, containerGetTitleNext, handle, (jint)flags), _DW_REFERENCE_NONE);
            if(jstr)
            {
                const char *str = env->GetStringUTFChars(jstr, nullptr);

                if(str)
                {
                    retval = strdup(str);
                    env->ReleaseStringUTFChars(jstr, str);
                }
            }
        }
    }
    return retval;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor(HWND handle, const char *text)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Generate a string
        jstring jstr = text ? env->NewStringUTF(text) : nullptr;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerCursor = env->GetMethodID(clazz, "containerCursor",
                                                               "(Landroid/widget/ListView;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerCursor, handle, jstr);
        _dw_jni_check_exception(env);
        if(jstr)
            env->DeleteLocalRef(jstr);
    }
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_cursor_by_data(HWND handle, void *data)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerCursorByData = env->GetMethodID(clazz, "containerCursorByData",
                                                              "(Landroid/widget/ListView;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerCursorByData, handle, (jlong)data);
        _dw_jni_check_exception(env);
    }
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, const char *text)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Generate a string
        jstring jstr = text ? env->NewStringUTF(text) : nullptr;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerRowDeleteByTitle = env->GetMethodID(clazz, "containerRowDeleteByTitle",
                                                               "(Landroid/widget/ListView;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerRowDeleteByTitle, handle, jstr);
        _dw_jni_check_exception(env);
        if(jstr)
            env->DeleteLocalRef(jstr);
    }
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_delete_row_by_data(HWND handle, void *data)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID containerRowDeleteByData = env->GetMethodID(clazz, "containerRowDeleteByData",
                                                              "(Landroid/widget/ListView;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, containerRowDeleteByData, handle, (jlong)data);
        _dw_jni_check_exception(env);
    }
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
    /* Not necessary with no columns */
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void API dw_taskbar_insert(HWND handle, HICN icon, const char *bubbletext)
{
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
}

HICN _dw_icon_load(const char *filename, const char *data, int len, int resid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a string
        jstring file = nullptr;
        if(filename)
            file = env->NewStringUTF(filename);
        // Construct a byte array
        jbyteArray bytearray = nullptr;
        if(data && len > 0)
        {
            bytearray = env->NewByteArray(len);
            env->SetByteArrayRegion(bytearray, 0, len, reinterpret_cast<const jbyte *>(data));
        }
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID iconNew = env->GetMethodID(clazz, "iconNew",
                                             "(Ljava/lang/String;[BII)Landroid/graphics/drawable/Drawable;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, iconNew,
                                              file, bytearray, len, resid), _DW_REFERENCE_STRONG);
        // TODO: Clean up after the array now that we are finished
        //if(bytearray)
        //env->ReleaseByteArrayElements(bytearray, (jbyte *) data, 0);
        if(file)
            env->DeleteLocalRef(file);
        return result;
    }
    return nullptr;
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 * Returns:
 *       Handle to the created icon or nullptr on error.
 */
HICN API dw_icon_load(unsigned long module, unsigned long resid)
{
    return _dw_icon_load(nullptr, nullptr, 0, resid);
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       Handle to the created icon or nullptr on error.
 */
HICN API dw_icon_load_from_file(const char *filename)
{
    return _dw_icon_load(filename, nullptr, 0, 0);
}

/*
 * Obtains an icon from data.
 * Parameters:
 *       data: Data for the icon (ICO on OS/2 or Windows, XPM on Unix, PNG on Mac)
 *       len: Length of the passed in data.
 * Returns:
 *       Handle to the created icon or nullptr on error.
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
    return _dw_icon_load(nullptr, data, len, 0);
}

/*
 * Frees a loaded icon resource.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
    if(handle)
    {
        JNIEnv *env;

        if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            env->DeleteGlobalRef(handle);
        }
    }
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 * Returns:
 *       Handle to the created MDI widget or nullptr on error.
 */
HWND API dw_mdi_new(unsigned long cid)
{
    return nullptr;
}

/*
 * Creates a splitbar window (widget) with given parameters.
 * Parameters:
 *       type: Value can be DW_VERT or DW_HORZ.
 *       topleft: Handle to the window to be top or left.
 *       bottomright:  Handle to the window to be bottom or right.
 * Returns:
 *       A handle to a splitbar window or nullptr on failure.
 */
HWND API dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID splitBarNew = env->GetMethodID(clazz, "splitBarNew",
                                                 "(ILandroid/view/View;Landroid/view/View;I)Landroidx/constraintlayout/widget/ConstraintLayout;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, splitBarNew, type, topleft, bottomright, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 *       percent: The position of the splitbar.
 */
void API dw_splitbar_set(HWND handle, float percent)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        jfloat position = (jfloat)percent;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID splitBarSet = env->GetMethodID(clazz, "splitBarSet",
                                                 "(Landroidx/constraintlayout/widget/ConstraintLayout;F)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, splitBarSet, handle, position);
        _dw_jni_check_exception(env);
    }
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 * Returns:
 *       Position of the splitbar (percentage).
 */
float API dw_splitbar_get(HWND handle)
{
    JNIEnv *env;
    float retval = 0;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID splitBarGet = env->GetMethodID(clazz, "splitBarGet",
                                                 "(Landroidx/constraintlayout/widget/ConstraintLayout;)F");
        // Call the method on the object
        retval = (float)env->CallFloatMethod(_dw_obj, splitBarGet, handle);
        if(_dw_jni_check_exception(env))
            retval = 0;
    }
    return retval;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created bitmap widget or nullptr on error.
 */
HWND API dw_bitmap_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID mleNew = env->GetMethodID(clazz, "bitmapNew",
                                            "(I)Landroid/widget/ImageView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, mleNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/* Unified internal function to create an Android internal Bitmap */
jobject _dw_jbitmap_new(unsigned long width, unsigned long height, const char *filename, const char *data, int len, int resid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a byte array
        jbyteArray bytearray = nullptr;
        if(data && len > 0)
        {
            bytearray = env->NewByteArray(len);
            env->SetByteArrayRegion(bytearray, 0, len, reinterpret_cast<const jbyte *>(data));
        }
        if(!_dw_jni_check_exception(env))
        {
            // Construct a string
            jstring file = nullptr;
            if(filename)
                file = env->NewStringUTF(filename);
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID pixmapNew = env->GetMethodID(clazz, "pixmapNew",
                                                   "(IILjava/lang/String;[BII)Landroid/graphics/Bitmap;");
            // Call the method on the object
            jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, pixmapNew,
                                                                             (jint) width,
                                                                             (jint) height,
                                                                             file, bytearray, len,
                                                                             resid),
                                                  _DW_REFERENCE_STRONG);
            // TODO: Clean up after the array now that we are finished
            //if(bytearray)
            //env->ReleaseByteArrayElements(bytearray, (jbyte *) data, 0);
            if(file)
                env->DeleteLocalRef(file);
            return result;
        }
    }
    return nullptr;
}

void _dw_pixmap_get_dimensions(HPIXMAP pixmap)
{
    JNIEnv *env;

    if(pixmap && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID pixmapGetDimensions = env->GetMethodID(clazz, "pixmapGetDimensions",
                                                         "(Landroid/graphics/Bitmap;)J");
        // Call the method on the object
        jlong dimensions = env->CallLongMethod(_dw_obj, pixmapGetDimensions, pixmap->bitmap);
        if(_dw_jni_check_exception(env))
            dimensions = 0;

        pixmap->width = dimensions & 0xFFFF;
        pixmap->height = (dimensions >> 32) & 0xFFFF;
    }
}

/*
 * Creates a pixmap with given parameters.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       width: Width of the pixmap in pixels.
 *       height: Height of the pixmap in pixels.
 *       depth: Color depth of the pixmap.
 * Returns:
 *       A handle to a pixmap or nullptr on failure.
 */
HPIXMAP API dw_pixmap_new(HWND handle, unsigned long width, unsigned long height, int depth)
{
    jobject jbitmap = _dw_jbitmap_new(width, height, nullptr, nullptr, 0, 0);

    if(jbitmap)
    {
        HPIXMAP pixmap = (HPIXMAP)calloc(1, sizeof(struct _hpixmap));

        if(pixmap)
        {
            pixmap->width = width;
            pixmap->height = height;
            pixmap->bitmap = jbitmap;
            pixmap->handle = handle;

            return pixmap;
        }
    }
    return nullptr;
}

/*
 * Creates a pixmap from a file.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a pixmap or nullptr on failure.
 */
HPIXMAP API dw_pixmap_new_from_file(HWND handle, const char *filename)
{
    jobject jbitmap = _dw_jbitmap_new(0, 0, filename, nullptr, 0, 0);

    if(jbitmap)
    {
        HPIXMAP pixmap = (HPIXMAP)calloc(1, sizeof(struct _hpixmap));

        if(pixmap)
        {
            pixmap->bitmap = jbitmap;
            pixmap->handle = handle;
            _dw_pixmap_get_dimensions(pixmap);

            return pixmap;
        }
    }
    return nullptr;
}

/*
 * Creates a pixmap from data in memory.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of the image data
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       len: Length of data
 * Returns:
 *       A handle to a pixmap or nullptr on failure.
 */
HPIXMAP API dw_pixmap_new_from_data(HWND handle, const char *data, int len)
{
    jobject jbitmap = _dw_jbitmap_new(0, 0, nullptr, data, len, 0);

    if(jbitmap)
    {
        HPIXMAP pixmap = (HPIXMAP)calloc(1, sizeof(struct _hpixmap));

        if(pixmap)
        {
            pixmap->bitmap = jbitmap;
            pixmap->handle = handle;
            _dw_pixmap_get_dimensions(pixmap);

            return pixmap;
        }
    }
    return nullptr;
}

/*
 * Sets the transparent color for a pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  Transparent RGB color
 * Note: This is only necessary on platforms that
 *       don't handle transparency automatically
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
    /* Not necessary on Android, all versions support transparency. */
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or nullptr on failure.
 */
HPIXMAP API dw_pixmap_grab(HWND handle, ULONG resid)
{
    jobject jbitmap = _dw_jbitmap_new(0, 0, nullptr, nullptr, 0, (int)resid);

    if(jbitmap)
    {
        HPIXMAP pixmap = (HPIXMAP)calloc(1, sizeof(struct _hpixmap));

        if(pixmap)
        {
            pixmap->bitmap = jbitmap;
            pixmap->handle = handle;
            _dw_pixmap_get_dimensions(pixmap);

            return pixmap;
        }
    }
    return nullptr;
}

/*
 * Sets the font used by a specified pixmap.
 * Normally the pixmap font is obtained from the associated window handle.
 * However this can be used to override that, or for pixmaps with no window.
 * Parameters:
 *          pixmap: Handle to a pixmap returned by dw_pixmap_new() or
 *                  passed to the application via a callback.
 *          fontname: Name and size of the font in the form "size.fontname"
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
*/
int API dw_pixmap_set_font(HPIXMAP pixmap, const char *fontname)
{
    JNIEnv *env;

    if(pixmap && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a string
        jstring jstr = env->NewStringUTF(fontname);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID typefaceFromFontName = env->GetMethodID(clazz, "typefaceFromFontName",
                                                          "(Ljava/lang/String;)Landroid/graphics/Typeface;");
        // Call the method on the object
        jobject typeface = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, typefaceFromFontName, jstr), _DW_REFERENCE_STRONG);
        if(typeface)
        {
            jobject oldtypeface = pixmap->typeface;
            pixmap->typeface = typeface;
            if(oldtypeface)
                env->DeleteGlobalRef(oldtypeface);
            pixmap->fontsize = atoi(fontname);
        }
        env->DeleteLocalRef(jstr);
        return DW_ERROR_NONE;
    }
    return DW_ERROR_GENERAL;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
void API dw_pixmap_destroy(HPIXMAP pixmap)
{
    if(pixmap)
    {
        JNIEnv *env;

        if(pixmap && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            env->DeleteGlobalRef(pixmap->bitmap);
            if(pixmap->typeface)
                env->DeleteGlobalRef(pixmap->typeface);
            free(pixmap);
        }
    }
}

/*
 * Returns the width of the pixmap, same as the DW_PIXMAP_WIDTH() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_width(HPIXMAP pixmap)
{
    return pixmap ? pixmap->width : 0;
}

/*
 * Returns the height of the pixmap, same as the DW_PIXMAP_HEIGHT() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_height(HPIXMAP pixmap)
{
    return pixmap ? pixmap->height : 0;
}

/*
 * Copies from one item to another.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of area to copy.
 *       height: Height of area to copy.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 */
void API dw_pixmap_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc)
{
    dw_pixmap_stretch_bitblt(dest, destp, xdest, ydest, width, height, src, srcp, xsrc, ysrc, -1, -1);
}

/*
 * Copies from one surface to another allowing for stretching.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of the target area.
 *       height: Height of the target area.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 *       srcwidth: Width of area to copy.
 *       srcheight: Height of area to copy.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_pixmap_stretch_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc, int srcwidth, int srcheight)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID pixmapBitBlt = env->GetMethodID(clazz, "pixmapBitBlt",
                                                  "(Lorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIIILorg/dbsoft/dwindows/DWRender;Landroid/graphics/Bitmap;IIII)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, pixmapBitBlt, dest, destp ? destp->bitmap : nullptr, xdest, ydest, width, height, src, srcp ? srcp->bitmap : nullptr, xsrc, ysrc, srcwidth, srcheight);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_GENERAL;
    }
    return retval;
}

/*
 * Create a new calendar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created calendar or nullptr on error.
 */
HWND API dw_calendar_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID calendarNew = env->GetMethodID(clazz, "calendarNew",
                                                 "(I)Landroid/widget/CalendarView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, calendarNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Sets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year, month, day: To set the calendar to display.
 */
void API dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        time_t date;
        struct tm ts = {0};

        // Convert to Unix time
        ts.tm_year = year - 1900;
        ts.tm_mon = month - 1;
        ts.tm_mday = day;
        date = mktime(&ts);

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID calendarSetDate = env->GetMethodID(clazz, "calendarSetDate",
                                                     "(Landroid/widget/CalendarView;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, calendarSetDate, handle, (jlong)date);
        _dw_jni_check_exception(env);
    }
}

/*
 * Gets the year, month and day set in the calendar widget.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year: Variable to store the year or nullptr.
 *       month: Variable to store the month or nullptr.
 *       day: Variable to store the day or nullptr.
 */
void API dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        time_t date;
        struct tm ts;

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID calendarGetDate = env->GetMethodID(clazz, "calendarGetDate",
                                                     "(Landroid/widget/CalendarView;)J");
        // Call the method on the object
        date = (time_t)env->CallLongMethod(_dw_obj, calendarGetDate, handle);
        if(_dw_jni_check_exception(env))
            date = 0;
        ts = *localtime(&date);
        if(year)
            *year = ts.tm_year + 1900;
        if(month)
            *month = ts.tm_mon + 1;
        if(day)
            *day = ts.tm_mday;
    }
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlAction = env->GetMethodID(clazz, "htmlAction",
                                                "(Landroid/webkit/WebView;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, htmlAction, handle, action);
        _dw_jni_check_exception(env);
    }
}

/*
 * Render raw HTML code in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       string: String buffer containing HTML code to
 *               be rendered.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_raw(HWND handle, const char *string)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if(handle && string && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(string);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlRaw = env->GetMethodID(clazz, "htmlRaw",
                                                 "(Landroid/webkit/WebView;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, htmlRaw, handle, jstr);
        if(!_dw_jni_check_exception(env))
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Render file or web page in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       url: Universal Resource Locator of the web or
 *               file object to be rendered.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_url(HWND handle, const char *url)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if(handle && url && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(url);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlLoadURL = env->GetMethodID(clazz, "htmlLoadURL",
                                                 "(Landroid/webkit/WebView;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, htmlLoadURL, handle, jstr);
        if(!_dw_jni_check_exception(env))
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Executes the javascript contained in "script" in the HTML window.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       script: Javascript code to execute.
 *       scriptdata: Data passed to the signal handler.
 * Notes: A DW_SIGNAL_HTML_RESULT event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_javascript_run(HWND handle, const char *script, void *scriptdata)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if(handle && script && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(script);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlJavascriptRun = env->GetMethodID(clazz, "htmlJavascriptRun",
                                                       "(Landroid/webkit/WebView;Ljava/lang/String;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, htmlJavascriptRun, handle, jstr, (jlong)scriptdata);
        if(!_dw_jni_check_exception(env))
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Install a javascript function with name that can call native code.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       name: Javascript function name.
 * Notes: A DW_SIGNAL_HTML_MESSAGE event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_javascript_add(HWND handle, const char *name)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && name && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(name);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlJavascriptAdd = env->GetMethodID(clazz, "htmlJavascriptAdd",
                                                       "(Landroid/webkit/WebView;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, htmlJavascriptAdd, handle, jstr);
        if(!_dw_jni_check_exception(env))
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Create a new HTML window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created html widget or nullptr on error.
 */
HWND API dw_html_new(unsigned long cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID htmlNew = env->GetMethodID(clazz, "htmlNew",
                                             "(I)Landroid/webkit/WebView;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, htmlNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate or nullptr.
 *       y: Pointer to variable to store Y coordinate or nullptr.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
    /* No mouse pointer on Android */
    if(x)
        *x = 0;
    if(y)
        *y = 0;
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_pointer_set_pos(long x, long y)
{
    /* No mouse pointer on Android */
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used associated with this menu.
 * Returns:
 *       Handle to the created menu or nullptr on error.
 */
HMENUI API dw_menu_new(ULONG cid)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuNew = env->GetMethodID(clazz, "menuNew",
                                             "(I)Lorg/dbsoft/dwindows/DWMenu;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, menuNew, (int)cid), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 * Returns:
 *       Handle to the created menu bar or nullptr on error.
 */
HMENUI API dw_menubar_new(HWND location)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuBarNew = env->GetMethodID(clazz, "menuBarNew",
                                                "(Landroid/view/View;)Lorg/dbsoft/dwindows/DWMenu;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, menuBarNew, location), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
    JNIEnv *env;

    if(menu && *menu && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuDestroy = env->GetMethodID(clazz, "menuDestroy",
                                                 "(Lorg/dbsoft/dwindows/DWMenu;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, menuDestroy, *menu);
        _dw_jni_check_exception(env);
        *menu = nullptr;
    }
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menux, unsigned long id)
{
    JNIEnv *env;

    if(menux && id > 0 && id < 30000 && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuDeleteItem = env->GetMethodID(clazz, "menuDeleteItem",
                                                    "(Lorg/dbsoft/dwindows/DWMenu;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, menuDeleteItem, menux, (int)id);
        if(!_dw_jni_check_exception(env))
            return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Pops up a context menu at given x and y coordinates.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       parent: Handle to the window initiating the popup.
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_menu_popup(HMENUI *menu, HWND parent, int x, int y)
{
    JNIEnv *env;

    if(menu && *menu && parent && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuPopup = env->GetMethodID(clazz, "menuPopup",
                                               "(Lorg/dbsoft/dwindows/DWMenu;Landroid/view/View;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, menuPopup, *menu, parent, x, y);
        _dw_jni_check_exception(env);
        *menu = nullptr;
    }
}

char _dw_removetilde(char *dest, const char *src)
{
    int z, cur=0;
    char accel = '\0';

    for(z=0;z<strlen(src);z++)
    {
        if(src[z] != '~')
        {
            dest[cur] = src[z];
            cur++;
        }
        else
        {
            accel = src[z+1];
        }
    }
    dest[cur] = 0;
    return accel;
}

/*
 * Adds a menuitem or submenu to an existing menu.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       submenu: Handle to an existing menu to be a submenu or nullptr.
 * Returns:
 *       Handle to the created menu item or nullptr on error.
 */
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG itemid, ULONG flags, int end, int check, HMENUI submenux)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        char *newtitle = (char *)alloca(strlen(title)+1);
        char accel = _dw_removetilde(newtitle, title);
        // Create a string
        jstring jstr = env->NewStringUTF(newtitle);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuAppendItem = env->GetMethodID(clazz, "menuAppendItem",
                                                    "(Lorg/dbsoft/dwindows/DWMenu;Ljava/lang/String;IIIILorg/dbsoft/dwindows/DWMenu;)Lorg/dbsoft/dwindows/DWMenuItem;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, menuAppendItem, menux, jstr, (int)itemid, (int)flags, end, check, submenux), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Sets the state of a menu item check.
 * Deprecated; use dw_menu_item_set_state()
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 */
void API dw_menu_item_set_check(HMENUI menux, unsigned long itemid, int check)
{
    dw_menu_item_set_state(menux, itemid, check ? DW_MIS_CHECKED : DW_MIS_UNCHECKED);
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       flags: DW_MIS_ENABLED/DW_MIS_DISABLED
 *              DW_MIS_CHECKED/DW_MIS_UNCHECKED
 */
void API dw_menu_item_set_state(HMENUI menux, unsigned long itemid, unsigned long state)
{
    JNIEnv *env;

    if(menux && itemid > 0 && itemid < 30000 && state && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID menuSetState = env->GetMethodID(clazz, "menuSetState",
                                                  "(Lorg/dbsoft/dwindows/DWMenu;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, menuSetState, menux, (int)itemid, (int)state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       Handle to the created notebook or nullptr on error.
 */
HWND API dw_notebook_new(ULONG cid, int top)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookNew = env->GetMethodID(clazz, "notebookNew",
                                                 "(II)Landroid/widget/RelativeLayout;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, notebookNew, (int)cid, top), _DW_REFERENCE_WEAK);
        return result;
    }
    return nullptr;
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 * Returns:
 *       ID of newly created notebook page.
 */
unsigned long API dw_notebook_page_new(HWND handle, ULONG flags, int front)
{
    JNIEnv *env;
    unsigned long result = 0;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPageNew = env->GetMethodID(clazz, "notebookPageNew",
                                                     "(Landroid/widget/RelativeLayout;JI)J");
        // Call the method on the object
        result = (unsigned long)env->CallLongMethod(_dw_obj, notebookPageNew, handle, (jlong)flags, front);
        if(_dw_jni_check_exception(env))
            result = 0;
    }
    return result;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned long pageid)
{
    JNIEnv *env;

    if(handle && pageid && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPageDestroy = env->GetMethodID(clazz, "notebookPageDestroy",
                                                         "(Landroid/widget/RelativeLayout;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, notebookPageDestroy, handle, (jlong)pageid);
        _dw_jni_check_exception(env);
    }
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 * Returns:
 *       ID of visible notebook page.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
    JNIEnv *env;
    unsigned long result = 0;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key))) {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPageGet = env->GetMethodID(clazz, "notebookPageGet",
                                                     "(Landroid/widget/RelativeLayout;)J");
        // Call the method on the object
        result = (unsigned long)env->CallLongMethod(_dw_obj, notebookPageGet, handle);
        if(_dw_jni_check_exception(env))
            result = 0;
    }
    return result;
}

/*
 * Sets the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned long pageid)
{
    JNIEnv *env;

    if(handle && pageid && (env = (JNIEnv *)pthread_getspecific(_dw_env_key))) {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPageSet = env->GetMethodID(clazz, "notebookPageSet",
                                                     "(Landroid/widget/RelativeLayout;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, notebookPageSet, handle, (jlong)pageid);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_text(HWND handle, ULONG pageid, const char *text)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPageSetText = env->GetMethodID(clazz, "notebookPageSetText",
                                                         "(Landroid/widget/RelativeLayout;JLjava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, notebookPageSetText, handle, (jlong)pageid, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_status_text(HWND handle, ULONG pageid, const char *text)
{
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
void API dw_notebook_pack(HWND handle, ULONG pageid, HWND page)
{
    JNIEnv *env;

    if(handle && pageid && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notebookPagePack = env->GetMethodID(clazz, "notebookPagePack",
                                                      "(Landroid/widget/RelativeLayout;JLandroid/view/View;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, notebookPagePack, handle, (jlong)pageid, page);
        _dw_jni_check_exception(env);
    }
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags.
 * Returns:
 *       Handle to the created window or nullptr on error.
 */
HWND API dw_window_new(HWND hwndOwner, const char *title, ULONG flStyle)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(title);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowNew = env->GetMethodID(clazz, "windowNew",
                                               "(Ljava/lang/String;I)Landroid/widget/LinearLayout;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, windowNew, jstr, (int)flStyle), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(jstr);
        return result;
    }
    return nullptr;
}

/*
 * Call a function from the window (widget)'s context (typically the message loop thread).
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
void API dw_window_function(HWND handle, void *function, void *data)
{
    void (* windowfunc)(void *);

    windowfunc = (void (*)(void *))function;

    if(windowfunc)
        windowfunc(data);
}


/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
    /* No mouse pointer on Android */
}

int _dw_window_hide_show(HWND handle, int state)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowHideShow = env->GetMethodID(clazz, "windowHideShow",
                                                    "(Landroid/view/View;I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowHideShow, handle, state);
        if(!_dw_jni_check_exception(env))
            return DW_ERROR_NONE;
    }
    return DW_ERROR_GENERAL;
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_show(HWND handle)
{
    return _dw_window_hide_show(handle, TRUE);
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to hide.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_hide(HWND handle)
{
    return _dw_window_hide_show(handle, FALSE);
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in DW_RGB format or a default color index.
 *          back: Background color in DW_RGB format or a default color index.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        unsigned long _fore = _dw_get_color(fore);
        unsigned long _back = _dw_get_color(back);

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetColor = env->GetMethodID(clazz, "windowSetColor",
                                                    "(Landroid/view/View;IIIIIIIIII)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetColor, handle,
                            (jint)fore, 0, (jint)DW_RED_VALUE(_fore), (jint)DW_GREEN_VALUE(_fore), (jint)DW_BLUE_VALUE(_fore),
                            (jint)back, 0, (jint)DW_RED_VALUE(_back), (jint)DW_GREEN_VALUE(_back), (jint)DW_BLUE_VALUE(_back));
        if(!_dw_jni_check_exception(env))
            return DW_ERROR_NONE;
    }
    return DW_ERROR_GENERAL;
}

/*
 * Sets the border size of a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_border(HWND handle, int border)
{
    /* No overlapping windows on Android, so no border */
    return DW_ERROR_GENERAL;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          style: Style features enabled or disabled.
 *          mask: Corresponding bitmask of features to be changed.
 */
void API dw_window_set_style(HWND handle, ULONG style, ULONG mask)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetStyle = env->GetMethodID(clazz, "windowSetStyle",
                                                    "(Ljava/lang/Object;II)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetStyle, handle, (jint)style, (jint)mask);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the current focus item for a window/dialog.
 * Parameters:
 *         handle: Handle to the dialog item to be focused.
 * Remarks:
 *          This is for use after showing the window/dialog.
 */
void API dw_window_set_focus(HWND handle)
{
    JNIEnv *env;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetFocus = env->GetMethodID(clazz, "windowSetFocus",
                                                    "(Landroid/view/View;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetFocus, handle);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND handle, HWND defaultitem)
{
    JNIEnv *env;

    if(handle && defaultitem && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowDefault = env->GetMethodID(clazz, "windowDefault",
                                                    "(Landroid/view/View;Landroid/view/View;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowDefault, handle, defaultitem);
        _dw_jni_check_exception(env);
    }
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND handle, HWND next)
{
    /* TODO: See if this is possible on Android */
}

/*
 * Captures the mouse input to this window even if it is outside the bounds.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
    /* No mouse pointer on Android */
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
    /* No mouse pointer on Android */
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
    /* TODO: Do we need to do implement this? */
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_font(HWND handle, const char *fontname)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a string
        jstring jstr = env->NewStringUTF(fontname);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetFont = env->GetMethodID(clazz, "windowSetFont",
                                                    "(Landroid/view/View;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetFont, handle, jstr);
        if(!_dw_jni_check_exception(env))
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Returns the current font for the specified window.
 * Parameters:
 *           handle: The window handle from which to obtain the font.
 * Returns:
 *       A malloc()ed font name string to be dw_free()ed or nullptr on error.
 */
char * API dw_window_get_font(HWND handle)
{
    JNIEnv *env;
    char *fontname = nullptr;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowGetFont = env->GetMethodID(clazz, "windowGetFont",
                                                   "(Landroid/view/View;)Ljava/lang/String;");
        // Call the method on the object
        jstring jstr = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, windowGetFont, handle), _DW_REFERENCE_NONE);

        if(jstr)
        {
            const char *str = env->GetStringUTFChars(jstr, nullptr);

            if(str)
            {
                fontname = strdup(str);
                env->ReleaseStringUTFChars(jstr, str);
            }
        }
    }
    return fontname;
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or nullptr on error.
 */
char * API dw_font_choose(const char *currfont)
{
    return nullptr;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
    /* TODO: Implement this */
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_destroy(HWND handle)
{
    JNIEnv *env;
    int retval = DW_ERROR_GENERAL;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowDestroy = env->GetMethodID(clazz, "windowDestroy",
                                                   "(Ljava/lang/Object;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, windowDestroy, handle);
        _dw_jni_check_exception(env);
    }
    return retval;
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       The text associsated with a given window or nullptr on error.
 */
char * API dw_window_get_text(HWND handle)
{
    JNIEnv *env;
    char *retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        const char *utf8 = nullptr;

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowGetText = env->GetMethodID(clazz, "windowGetText",
                                                   "(Landroid/view/View;)Ljava/lang/String;");
        // Call the method on the object
        jstring result = (jstring)_dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, windowGetText, handle), _DW_REFERENCE_NONE);
        // Get the UTF8 string result
        if(result)
            utf8 = env->GetStringUTFChars(result, nullptr);
        if(utf8)
        {
            retval = strdup(utf8);
            env->ReleaseStringUTFChars(result, utf8);
        }
    }
    return retval;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, const char *text)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(text);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetText = env->GetMethodID(clazz, "windowSetText",
                                                   "(Landroid/view/View;Ljava/lang/String;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetText, handle, jstr);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, const char *bubbletext)
{
    /* No mouse pointer on Android */
}

void _dw_window_set_enabled(HWND handle, jboolean state)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetEnabled = env->GetMethodID(clazz, "windowSetEnabled",
                                                      "(Landroid/view/View;Z)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetEnabled, handle, state);
        _dw_jni_check_exception(env);
    }
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
    _dw_window_set_enabled(handle, JNI_FALSE);
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
    _dw_window_set_enabled(handle, JNI_TRUE);
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       data: memory buffer containing image (Bitmap on OS/2 or
 *                 Windows and a pixmap on Unix, pass
 *                 nullptr if you use the id param)
 *       len: Length of data passed
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long cid, const char *data, int len)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a byte array
        jbyteArray bytearray = nullptr;
        if(data && len > 0)
        {
            bytearray = env->NewByteArray(len);
            env->SetByteArrayRegion(bytearray, 0, len, reinterpret_cast<const jbyte *>(data));
        }
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetBitmapFromData = env->GetMethodID(clazz, "windowSetBitmapFromData",
                                                             "(Landroid/view/View;I[BI)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, windowSetBitmapFromData, handle, (int)cid, bytearray, len);
        _dw_jni_check_exception(env);
        // Clean up after the array now that we are finished
        //if(bytearray)
            //env->ReleaseByteArrayElements(bytearray, (jbyte *) data, 0);
    }
    return retval;
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       filename: a path to a file (Bitmap on OS/2 or
 *                 Windows and a pixmap on Unix, pass
 *                 nullptr if you use the id param)
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap(HWND handle, unsigned long resid, const char *filename)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a string
        jstring jstr = nullptr;
        if(filename) {
            jstr = env->NewStringUTF(filename);
        }
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetBitmapFromData = env->GetMethodID(clazz, "windowSetBitmap",
                                                             "(Landroid/view/View;ILjava/lang/String;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, windowSetBitmapFromData, handle, (int)resid, jstr);
        _dw_jni_check_exception(env);
        if(jstr)
            env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       icon: Handle to icon to be used.
 */
void API dw_window_set_icon(HWND handle, HICN icon)
{
    /* TODO: See if this is possible, the icon is controlled by app resources */
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 * Returns:
 *       HWND of window with ID or nullptr on error.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
    JNIEnv *env;
    HWND retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowFromId = env->GetMethodID(clazz, "windowFromId",
                                                  "(Landroid/view/View;I)Landroid/view/View;");
        // Call the method on the object
        retval = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, windowFromId, handle, id), _DW_REFERENCE_NONE);
    }
    return retval;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_minimize(HWND handle)
{
    /* TODO: Not sure if we should do anything here */
    return DW_ERROR_GENERAL;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
    /* Shouldn't be necessary on Android */
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_raise(HWND handle)
{
    /* TODO: Not sure if we should do anything here */
    return DW_ERROR_GENERAL;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_lower(HWND handle)
{
    /* TODO: Not sure if we should do anyting here */
    return DW_ERROR_GENERAL;
}

/*
 * Sets the size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void API dw_window_set_size(HWND handle, ULONG width, ULONG height)
{
    /* Not possible on Android */
}

/*
 * Gets the size the system thinks the widget should be.
 * Parameters:
 *       handle: Window (widget) handle of the item to query.
 *       width: Width in pixels of the item or nullptr if not needed.
 *       height: Height in pixels of the item or nullptr if not needed.
 */
void API dw_window_get_preferred_size(HWND handle, int *width, int *height)
{
    if(width || height)
    {
        JNIEnv *env;

        if(handle && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID windowGetPreferredSize = env->GetMethodID(clazz, "windowGetPreferredSize",
                                                                "(Landroid/view/View;)J");
            // Call the method on the object
            jlong dimensions = env->CallLongMethod(_dw_obj, windowGetPreferredSize, handle);
            if(_dw_jni_check_exception(env))
                dimensions = 0;
            if(width)
                *width = (int)(dimensions & 0xFFFF);
            if(height)
                *height = (int)((dimensions >> 32) & 0xFFFF);
        }
        else
        {
            if(width)
                *width = 0;
            if(height)
                *height = 0;
        }
    }
}

/*
 * Sets the gravity of a given window (widget).
 * Gravity controls which corner of the screen and window the position is relative to.
 * Parameters:
 *          handle: Window (widget) handle.
 *          horz: DW_GRAV_LEFT (default), DW_GRAV_RIGHT or DW_GRAV_CENTER.
 *          vert: DW_GRAV_TOP (default), DW_GRAV_BOTTOM or DW_GRAV_CENTER.
 */
void API dw_window_set_gravity(HWND handle, int horz, int vert)
{
    /* This is not possible on Android */
}

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
void API dw_window_set_pos(HWND handle, LONG x, LONG y)
{
    /* This is not possible on Android */
}

/*
 * Sets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
void API dw_window_set_pos_size(HWND handle, LONG x, LONG y, ULONG width, ULONG height)
{
    /* This is not possible on Android */
}

/*
 * Gets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left or nullptr.
 *          y: Y location from the bottom left or nullptr.
 *          width: Width of the widget or nullptr.
 *          height: Height of the widget or nullptr.
 */
void API dw_window_get_pos_size(HWND handle, LONG *x, LONG *y, ULONG *width, ULONG *height)
{
    if(x)
        *x = 0;
    if(y)
        *y = 0;
    if(width)
        *width = 0;
    if(height)
        *height = 0;
}

static jlong _dw_screen_dimensions = 0;

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
    if(!_dw_screen_dimensions)
    {
        JNIEnv *env;

        if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID screenGetDimensions = env->GetMethodID(clazz, "screenGetDimensions",
                                                             "()J");
            // Call the method on the object
            _dw_screen_dimensions = env->CallLongMethod(_dw_obj, screenGetDimensions);
            if(_dw_jni_check_exception(env))
                _dw_screen_dimensions = 0;
        }
    }
    return _dw_screen_dimensions & 0xFFFF;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
    if(!_dw_screen_dimensions)
    {
        JNIEnv *env;

        if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID screenGetDimensions = env->GetMethodID(clazz, "screenGetDimensions",
                                                             "()J");
            // Call the method on the object
            _dw_screen_dimensions = env->CallLongMethod(_dw_obj, screenGetDimensions);
            if(_dw_jni_check_exception(env))
                _dw_screen_dimensions = 0;
        }
    }
    return (_dw_screen_dimensions >> 32) & 0xFFFF;
}

/* This should return the current color depth. */
unsigned long API dw_color_depth_get(void)
{
    return 32;
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
    static char osName[_DW_ENV_STRING_SIZE+1] = { 0 };

    if(!osName[0])
    {
        JNIEnv *jenv;
        const char *release = nullptr;
        jstring jstr = nullptr;

        if((jenv = (JNIEnv *)pthread_getspecific(_dw_env_key)))
        {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(jenv, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID androidGetRelease = jenv->GetMethodID(clazz, "androidGetRelease",
                                                           "()Ljava/lang/String;");
            // Call the method on the object
            jstr = (jstring)_dw_jni_check_result(jenv, jenv->CallObjectMethod(_dw_obj, androidGetRelease), _DW_REFERENCE_NONE);

            if(jstr)
                release = jenv->GetStringUTFChars(jstr, nullptr);
        }
        snprintf(osName, _DW_ENV_STRING_SIZE-1, "Android%s%s",
                 release ? " " : "", release ? release : "");
        if(release)
            jenv->ReleaseStringUTFChars(jstr, release);
    }
    memset(env, '\0', sizeof(DWEnv));

    strncpy(env->osName, osName, sizeof(env->osName)-1);
    strncpy(env->buildDate, __DATE__, sizeof(env->buildDate)-1);
    strncpy(env->buildTime, __TIME__, sizeof(env->buildTime)-1);
    strncpy(env->htmlEngine, "CHROME", sizeof(env->htmlEngine)-1);
    env->DWMajorVersion = DW_MAJOR_VERSION;
    env->DWMinorVersion = DW_MINOR_VERSION;
    env->DWSubVersion = DW_SUB_VERSION;

    /* Operating system major */
    env->MajorVersion = _dw_android_api;
    /* Operating system minor */
    env->MinorVersion = 0;
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID doBeep = env->GetMethodID(clazz, "doBeep", "(I)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, doBeep, dur);
        _dw_jni_check_exception(env);
    }
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void API dw_flush(void)
{
}

/*
 * Add a named user data item to a window handle.
 * Parameters:
 *       window: Window handle to save data to.
 *       dataname: A string pointer identifying which data to be saved.
 *       data: User data to be saved to the window handle.
 */
void API dw_window_set_data(HWND window, const char *dataname, void *data)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(dataname);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowSetData = env->GetMethodID(clazz, "windowSetData",
                                                   "(Landroid/view/View;Ljava/lang/String;J)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, windowSetData, window, jstr, (jlong)data);
        _dw_jni_check_exception(env);
        env->DeleteLocalRef(jstr);
    }
}

/*
 * Gets a named user data item from a window handle.
 * Parameters:
 *       window: Window handle to get data from.
 *       dataname: A string pointer identifying which data to get.
 * Returns:
 *       Pointer to data or nullptr if no data is available.
 */
void * API dw_window_get_data(HWND window, const char *dataname)
{
    JNIEnv *env;
    void *retval = nullptr;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(dataname);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID windowGetData = env->GetMethodID(clazz, "windowGetData",
                                                   "(Landroid/view/View;Ljava/lang/String;)J");
        // Call the method on the object
        retval = (void *)env->CallLongMethod(_dw_obj, windowGetData, window, jstr);
        if(_dw_jni_check_exception(env))
            retval = nullptr;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Compare two window handles.
 * Parameters:
 *       window1: First window handle to compare.
 *       window2: Second window handle to compare.
 * Returns:
 *       TRUE if the windows are the same object, FALSE if not.
 */
int API dw_window_compare(HWND window1, HWND window2)
{
    JNIEnv *env;
    void *retval = nullptr;

    if(window1 && window2 && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        if(env->IsSameObject(window1, window2))
            return TRUE;
    }
    return FALSE;
}

/*
 * Add a callback to a timer event.
 * Parameters:
 *       interval: Milliseconds to delay between calls.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 * Returns:
 *       Timer ID for use with dw_timer_disconnect(), 0 on error.
 */
HTIMER API dw_timer_connect(int interval, void *sigfunc, void *data)
{
    JNIEnv *env;
    HTIMER retval = 0;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Use a long paramater
        jlong longinterval = (jlong)interval;
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID timerConnect = env->GetMethodID(clazz, "timerConnect",
                                                  "(JJJ)Ljava/util/Timer;");
        // Call the method on the object
        retval = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj,
                                    timerConnect, longinterval, (jlong)sigfunc, (jlong)data), _DW_REFERENCE_STRONG);
    }
    return retval;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(HTIMER timerid)
{
    JNIEnv *env;

    if(timerid && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID timerDisconnect = env->GetMethodID(clazz, "timerDisconnect",
                                                     "(Ljava/util/Timer;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, timerDisconnect, timerid);
        _dw_jni_check_exception(env);
    }
}

/*
 * Add a callback to a window event.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect(HWND window, const char *signame, void *sigfunc, void *data)
{
    dw_signal_connect_data(window, signame, sigfunc, nullptr, data);
}

/*
 * Add a callback to a window event with a closure callback.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       discfunc: The pointer to the function called when this handler is removed.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect_data(HWND window, const char *signame, void *sigfunc, void *discfunc, void *data)
{
    ULONG message = 0, msgid = 0;

    if(window && signame && sigfunc)
    {
        if((message = _dw_findsigmessage(signame)) != 0)
        {
            _dw_new_signal(message, window, (int)msgid, sigfunc, discfunc, data);
        }
    }
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_name(HWND window, const char *signame)
{
    DWSignalHandler *prev = nullptr, *tmp = DWRoot;
    ULONG message;

    if(!window || !signame || (message = _dw_findsigmessage(signame)) == 0)
        return;

    while(tmp)
    {
        if(tmp->window == window && tmp->message == message)
        {
            void (*discfunc)(HWND, void *) = (void (*)(HWND, void*))tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_window(HWND window)
{
    DWSignalHandler *prev = nullptr, *tmp = DWRoot;

    while(tmp)
    {
        if(tmp->window == window)
        {
            void (*discfunc)(HWND, void *) = (void (*)(HWND, void*))tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void API dw_signal_disconnect_by_data(HWND window, void *data)
{
    DWSignalHandler *prev = nullptr, *tmp = DWRoot;

    while(tmp)
    {
        if(tmp->window == window && tmp->data == data)
        {
            void (*discfunc)(HWND, void *) = (void (*)(HWND, void*))tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

void _dw_strlwr(char *buf)
{
    int z, len = strlen(buf);

    for(z=0;z<len;z++)
    {
        if(buf[z] >= 'A' && buf[z] <= 'Z')
            buf[z] -= 'A' - 'a';
    }
}

/* Open a shared library and return a handle.
 * Parameters:
 *         name: Base name of the shared library.
 *         handle: Pointer to a module handle,
 *                 will be filled in with the handle.
 */
int API dw_module_load(const char *name, HMOD *handle)
{
    int len;
    char *newname;
    char errorbuf[1025] = {0};


    if(!handle)
        return -1;

    if((len = strlen(name)) == 0)
        return   -1;

    /* Lenth + "lib" + ".so" + nullptr */
    newname = (char *)malloc(len + 7);

    if(!newname)
        return -1;

    sprintf(newname, "lib%s.so", name);
    _dw_strlwr(newname);

    *handle = dlopen(newname, RTLD_NOW);
    if(*handle == nullptr)
    {
        strncpy(errorbuf, dlerror(), 1024);
        printf("%s\n", errorbuf);
        sprintf(newname, "lib%s.so", name);
        *handle = dlopen(newname, RTLD_NOW);
    }

    free(newname);

    return (nullptr == *handle) ? -1 : 0;
}

/* Queries the address of a symbol within open handle.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 *         name: Name of the symbol you want the address of.
 *         func: A pointer to a function pointer, to obtain
 *               the address.
 */
int API dw_module_symbol(HMOD handle, const char *name, void**func)
{
    if(!func || !name)
        return   -1;

    if(strlen(name) == 0)
        return   -1;

    *func = (void*)dlsym(handle, name);
    return   (nullptr == *func);
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int API dw_module_close(HMOD handle)
{
    if(handle)
        return dlclose(handle);
    return 0;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
{
    HMTX mutex = (HMTX)malloc(sizeof(pthread_mutex_t));

    pthread_mutex_init(mutex, nullptr);
    return mutex;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
    if(mutex)
    {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }
}

/* Check if we are on the UI thread */
int _dw_is_ui_thread(void)
{
    JNIEnv *env;
    int retval = FALSE;
    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID isUIThread = env->GetMethodID(clazz, "isUIThread",
                                                "()Z");
        // Call the method on the object
        retval = env->CallBooleanMethod(_dw_obj, isUIThread);
        if(_dw_jni_check_exception(env))
            retval = FALSE;
    }
    return retval;
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_lock(HMTX mutex)
{
    /* We need to handle locks from the main thread differently...
     * since we can't stop message processing... otherwise we
     * will deadlock... so try to acquire the lock and continue
     * processing messages in between tries.
     */
    if(_dw_is_ui_thread())
    {
        while(pthread_mutex_trylock(mutex) != 0)
        {
            /* Process any pending events */
            dw_main_iteration();
            sched_yield();
        }
    }
    else
    {
        pthread_mutex_lock(mutex);
    }
}

/*
 * Tries to gain access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 * Returns:
 *       DW_ERROR_NONE on success, DW_ERROR_TIMEOUT if it is already locked.
 */
int API dw_mutex_trylock(HMTX mutex)
{
    if(pthread_mutex_trylock(mutex) == 0)
        return DW_ERROR_NONE;
    return DW_ERROR_TIMEOUT;
}

/*
 * Reliquishes the access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_unlock(HMTX mutex)
{
    pthread_mutex_unlock(mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
{
    HEV eve = (HEV)malloc(sizeof(struct _dw_unix_event));

    if(!eve)
        return nullptr;

    /* We need to be careful here, mutexes on Linux are
     * FAST by default but are error checking on other
     * systems such as FreeBSD and OS/2, perhaps others.
     */
    pthread_mutex_init (&(eve->mutex), nullptr);
    pthread_mutex_lock (&(eve->mutex));
    pthread_cond_init (&(eve->event), nullptr);

    pthread_mutex_unlock (&(eve->mutex));
    eve->alive = 1;
    eve->posted = 0;

    return eve;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_reset (HEV eve)
{
    if(!eve)
        return DW_ERROR_NON_INIT;

    pthread_mutex_lock (&(eve->mutex));
    pthread_cond_broadcast (&(eve->event));
    pthread_cond_init (&(eve->event), nullptr);
    eve->posted = 0;
    pthread_mutex_unlock (&(eve->mutex));
    return DW_ERROR_NONE;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_post (HEV eve)
{
    if(!eve)
        return DW_ERROR_NON_INIT;

    pthread_mutex_lock (&(eve->mutex));
    pthread_cond_broadcast (&(eve->event));
    eve->posted = 1;
    pthread_mutex_unlock (&(eve->mutex));
    return DW_ERROR_NONE;
}

/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_wait(HEV eve, unsigned long timeout)
{
    int rc;

    if(!eve)
        return DW_ERROR_NON_INIT;

    pthread_mutex_lock (&(eve->mutex));

    if(eve->posted)
    {
        pthread_mutex_unlock (&(eve->mutex));
        return DW_ERROR_NONE;
    }

    if(timeout != -1)
    {
        struct timeval now;
        struct timespec timeo;

        gettimeofday(&now, nullptr);
        timeo.tv_sec = now.tv_sec + (timeout / 1000);
        timeo.tv_nsec = now.tv_usec * 1000;
        rc = pthread_cond_timedwait(&(eve->event), &(eve->mutex), &timeo);
    }
    else
        rc = pthread_cond_wait(&(eve->event), &(eve->mutex));

    pthread_mutex_unlock (&(eve->mutex));
    if(!rc)
        return DW_ERROR_NONE;
    if(rc == ETIMEDOUT)
        return DW_ERROR_TIMEOUT;
    return DW_ERROR_GENERAL;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_close(HEV *eve)
{
    if(!eve || !(*eve))
        return DW_ERROR_NON_INIT;

    pthread_mutex_lock (&((*eve)->mutex));
    pthread_cond_destroy (&((*eve)->event));
    pthread_mutex_unlock (&((*eve)->mutex));
    pthread_mutex_destroy (&((*eve)->mutex));
    free(*eve);
    *eve = nullptr;

    return DW_ERROR_NONE;
}

struct _dw_seminfo {
    int fd;
    int waiting;
};

static void _dw_handle_sem(int *tmpsock)
{
    fd_set rd;
    struct _dw_seminfo *array = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo));
    int listenfd = tmpsock[0];
    int bytesread, connectcount = 1, maxfd, z, posted = 0;
    char command;
    sigset_t mask;

    sigfillset(&mask); /* Mask all allowed signals */
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    /* problems */
    if(tmpsock[1] == -1)
    {
        free(array);
        return;
    }

    array[0].fd = tmpsock[1];
    array[0].waiting = 0;

    /* Free the memory allocated in dw_named_event_new. */
    free(tmpsock);

    while(1)
    {
        FD_ZERO(&rd);
        FD_SET(listenfd, &rd);
        int DW_UNUSED(result);

        maxfd = listenfd;

        /* Added any connections to the named event semaphore */
        for(z=0;z<connectcount;z++)
        {
            if(array[z].fd > maxfd)
                maxfd = array[z].fd;

            FD_SET(array[z].fd, &rd);
        }

        if(select(maxfd+1, &rd, nullptr, nullptr, nullptr) == -1)
        {
            free(array);
            return;
        }

        if(FD_ISSET(listenfd, &rd))
        {
            struct _dw_seminfo *newarray;
            int newfd = accept(listenfd, nullptr, nullptr);

            if(newfd > -1)
            {
                /* Add new connections to the set */
                newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount+1));
                memcpy(newarray, array, sizeof(struct _dw_seminfo)*(connectcount));

                newarray[connectcount].fd = newfd;
                newarray[connectcount].waiting = 0;

                connectcount++;

                /* Replace old array with new one */
                free(array);
                array = newarray;
            }
        }

        /* Handle any events posted to the semaphore */
        for(z=0;z<connectcount;z++)
        {
            if(FD_ISSET(array[z].fd, &rd))
            {
                if((bytesread = read(array[z].fd, &command, 1)) < 1)
                {
                    struct _dw_seminfo *newarray;

                    /* Remove this connection from the set */
                    newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount-1));
                    if(!z)
                        memcpy(newarray, &array[1], sizeof(struct _dw_seminfo)*(connectcount-1));
                    else
                    {
                        memcpy(newarray, array, sizeof(struct _dw_seminfo)*z);
                        if(z!=(connectcount-1))
                            memcpy(&newarray[z], &array[z+1], sizeof(struct _dw_seminfo)*(connectcount-(z+1)));
                    }
                    connectcount--;

                    /* Replace old array with new one */
                    free(array);
                    array = newarray;
                }
                else if(bytesread == 1)
                {
                    switch(command)
                    {
                        case 0:
                        {
                            /* Reset */
                            posted = 0;
                        }
                            break;
                        case 1:
                            /* Post */
                        {
                            int s;
                            char tmp = (char)0;

                            posted = 1;

                            for(s=0;s<connectcount;s++)
                            {
                                /* The semaphore has been posted so
                                 * we tell all the waiting threads to
                                 * continue.
                                 */
                                if(array[s].waiting)
                                    result = write(array[s].fd, &tmp, 1);
                            }
                        }
                            break;
                        case 2:
                            /* Wait */
                        {
                            char tmp = (char)0;

                            array[z].waiting = 1;

                            /* If we are posted exit immeditately */
                            if(posted)
                                result = write(array[z].fd, &tmp, 1);
                        }
                            break;
                        case 3:
                        {
                            /* Done Waiting */
                            array[z].waiting = 0;
                        }
                            break;
                    }
                }
            }
        }
    }
}

/* Using domain sockets on unix for IPC */
/* Create a named event semaphore which can be
 * opened from other processes.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_new(const char *name)
{
    struct sockaddr_un un = {0};
    int ev, *tmpsock = (int *)malloc(sizeof(int)*2);
    DWTID dwthread;

    if(!tmpsock)
        return nullptr;

    tmpsock[0] = socket(AF_UNIX, SOCK_STREAM, 0);
    ev = socket(AF_UNIX, SOCK_STREAM, 0);
    un.sun_family=AF_UNIX;
    mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
    strcpy(un.sun_path, "/tmp/.dw/");
    strcat(un.sun_path, name);

    /* just to be safe, this should be changed
     * to support multiple instances.
     */
    remove(un.sun_path);

    bind(tmpsock[0], (struct sockaddr *)&un, sizeof(un));
    listen(tmpsock[0], 0);
    connect(ev, (struct sockaddr *)&un, sizeof(un));
    tmpsock[1] = accept(tmpsock[0], nullptr, nullptr);

    if(tmpsock[0] < 0 || tmpsock[1] < 0 || ev < 0)
    {
        if(tmpsock[0] > -1)
            close(tmpsock[0]);
        if(tmpsock[1] > -1)
            close(tmpsock[1]);
        if(ev > -1)
            close(ev);
        free(tmpsock);
        return nullptr;
    }

    /* Create a thread to handle this event semaphore */
    pthread_create(&dwthread, nullptr, (void *(*)(void *))_dw_handle_sem, (void *)tmpsock);
    return (HEV)DW_INT_TO_POINTER(ev);
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_get(const char *name)
{
    struct sockaddr_un un;
    int ev = socket(AF_UNIX, SOCK_STREAM, 0);
    if(ev < 0)
        return nullptr;

    un.sun_family=AF_UNIX;
    mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
    strcpy(un.sun_path, "/tmp/.dw/");
    strcat(un.sun_path, name);
    connect(ev, (struct sockaddr *)&un, sizeof(un));
    return (HEV)DW_INT_TO_POINTER(ev);
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_reset(HEV eve)
{
    /* signal reset */
    char tmp = (char)0;

    if(DW_POINTER_TO_INT(eve) < 0)
        return DW_ERROR_NONE;

    if(write(DW_POINTER_TO_INT(eve), &tmp, 1) == 1)
        return DW_ERROR_NONE;
    return DW_ERROR_GENERAL;
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_post(HEV eve)
{

    /* signal post */
    char tmp = (char)1;

    if(DW_POINTER_TO_INT(eve) < 0)
        return DW_ERROR_NONE;

    if(write(DW_POINTER_TO_INT(eve), &tmp, 1) == 1)
        return DW_ERROR_NONE;
    return DW_ERROR_GENERAL;
}

/* Waits on the specified semaphore until it becomes
 * posted, or returns immediately if it already is posted.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 *         timeout: Number of milliseconds before timing out
 *                  or -1 if indefinite.
 */
int API dw_named_event_wait(HEV eve, unsigned long timeout)
{
    fd_set rd;
    struct timeval tv, *useme = nullptr;
    int retval = 0;
    char tmp;

    if(DW_POINTER_TO_INT(eve) < 0)
        return DW_ERROR_NON_INIT;

    /* Set the timout or infinite */
    if(timeout != -1)
    {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout % 1000;

        useme = &tv;
    }

    FD_ZERO(&rd);
    FD_SET(DW_POINTER_TO_INT(eve), &rd);

    /* Signal wait */
    tmp = (char)2;
    retval = write(DW_POINTER_TO_INT(eve), &tmp, 1);

    if(retval == 1)
        retval = select(DW_POINTER_TO_INT(eve)+1, &rd, nullptr, nullptr, useme);

    /* Signal done waiting. */
    tmp = (char)3;
    if(retval == 1)
        retval = write(DW_POINTER_TO_INT(eve), &tmp, 1);

    if(retval == 0)
        return DW_ERROR_TIMEOUT;
    else if(retval == -1)
        return DW_ERROR_INTERRUPT;

    /* Clear the entry from the pipe so
     * we don't loop endlessly. :)
     */
    if(read(DW_POINTER_TO_INT(eve), &tmp, 1) == 1)
        return DW_ERROR_NONE;
    return DW_ERROR_GENERAL;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_close(HEV eve)
{
    /* Finally close the domain socket,
     * cleanup will continue in _dw_handle_sem.
     */
    close(DW_POINTER_TO_INT(eve));
    return DW_ERROR_NONE;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
    JNIEnv *env;

    _dw_jvm->AttachCurrentThread(&env, nullptr);
    pthread_setspecific(_dw_env_key, env);
    pthread_setspecific(_dw_fgcolor_key, nullptr);
    pthread_setspecific(_dw_bgcolor_key, (void *)DW_RGB_TRANSPARENT);
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
    _dw_jvm->DetachCurrentThread();
}

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
    void (*threadfunc)(void *) = nullptr;
    void **tmp = (void **)data;

    threadfunc = (void (*)(void *))tmp[0];

    /* Initialize colors */
    _dw_init_thread();

    threadfunc(tmp[1]);
    free(tmp);

    /* Free colors */
    _dw_deinit_thread();
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         handle: A pointer to receive a SHM identifier.
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 */
HSHM API dw_named_memory_new(void **dest, int size, const char *name)
{
    char namebuf[1025];
    struct _dw_unix_shm *handle = (struct _dw_unix_shm *)malloc(sizeof(struct _dw_unix_shm));

    mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
    snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

    if((handle->fd = open(namebuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
    {
        free(handle);
        return nullptr;
    }

    if(ftruncate(handle->fd, size))
    {
        close(handle->fd);
        free(handle);
        return nullptr;
    }

    /* attach the shared memory segment to our process's address space. */
    *dest = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

    if(*dest == MAP_FAILED)
    {
        close(handle->fd);
        *dest = nullptr;
        free(handle);
        return nullptr;
    }

    handle->size = size;
    handle->sid = getsid(0);
    handle->path = strdup(namebuf);

    return handle;
}

/*
 * Aquires shared memory region with a name.
 * Parameters:
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to requested.
 *         name: A string pointer to a unique memory name.
 */
HSHM API dw_named_memory_get(void **dest, int size, const char *name)
{
    char namebuf[1025];
    struct _dw_unix_shm *handle = (struct _dw_unix_shm *)malloc(sizeof(struct _dw_unix_shm));

    mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
    snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

    if((handle->fd = open(namebuf, O_RDWR)) < 0)
    {
        free(handle);
        return nullptr;
    }

    /* attach the shared memory segment to our process's address space. */
    *dest = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

    if(*dest == MAP_FAILED)
    {
        close(handle->fd);
        *dest = nullptr;
        free(handle);
        return nullptr;
    }

    handle->size = size;
    handle->sid = -1;
    handle->path = nullptr;

    return handle;
}

/*
 * Frees a shared memory region previously allocated.
 * Parameters:
 *         handle: Handle obtained from DB_named_memory_allocate.
 *         ptr: The memory address aquired with DB_named_memory_allocate.
 */
int API dw_named_memory_free(HSHM handle, void *ptr)
{
    struct _dw_unix_shm *h = (struct _dw_unix_shm *)handle;
    int rc = munmap(ptr, h->size);

    close(h->fd);
    if(h->path)
    {
        /* Only remove the actual file if we are the
         * creator of the file.
         */
        if(h->sid != -1 && h->sid == getsid(0))
            remove(h->path);
        free(h->path);
    }
    return rc;
}
/*
 * Creates a new thread with a starting point of func.
 * Parameters:
 *       func: Function which will be run in the new thread.
 *       data: Parameter(s) passed to the function.
 *       stack: Stack size of new thread (OS/2 and Windows only).
 */
DWTID API dw_thread_new(void *func, void *data, int stack)
{
    DWTID dwthread;
    void **tmp = (void **)malloc(sizeof(void *) * 2);
    int rc;

    tmp[0] = func;
    tmp[1] = data;

    rc = pthread_create(&dwthread, nullptr, (void *(*)(void *))_dwthreadstart, (void *)tmp);
    if(rc == 0)
        return dwthread;
    return (DWTID)DW_ERROR_UNKNOWN;
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
    pthread_exit(nullptr);
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
    return (DWTID)pthread_self();
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 *           argc: Passed in from main()
 *           argv: Passed in from main()
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
    JNIEnv *env;

    if(!_dw_app_id[0])
    {
        /* Generate an Application ID based on the PID if all else fails. */
        snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, getpid());
    }
    if(!_dw_app_name[0])
    {
        const char *name = getprogname();

        if(name)
            strncpy(_dw_app_name, name, _DW_APP_ID_SIZE);
    }

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring appid = env->NewStringUTF(_dw_app_id);
        jstring appname = env->NewStringUTF(_dw_app_name);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID dwInit = env->GetMethodID(clazz, "dwInit",
                                            "(Ljava/lang/String;Ljava/lang/String;)I");
        // Call the method on the object
        _dw_android_api = env->CallIntMethod(_dw_obj, dwInit, appid, appname);
        if(_dw_jni_check_exception(env))
            _dw_android_api = 0;
        env->DeleteLocalRef(appid);
        env->DeleteLocalRef(appname);
    }
    return DW_ERROR_NONE;
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID dwindowsShutdown = env->GetMethodID(clazz, "dwindowsShutdown",
                                                      "()V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, dwindowsShutdown);
        _dw_jni_check_exception(env);
    }
}

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       Process ID on success or DW_ERROR_UNKNOWN (-1) on error.
 */
#if defined(__ANDROID__) && (__ANDROID_API__+0) < 28
int API dw_exec(const char *program, int type, char **params)
{
    int retval = DW_ERROR_UNKNOWN;

    /* Forking isn't recommended on Android... but it works...
     * and prior to API 28 it is the only way to launch processes.
     * Type is ignored, since this can only launch background processes.
     */
    if((retval = fork()) == 0)
    {
        int i;

        /* Close any forked file descriptors */
        for(i=3; i<256; i++)
            close(i);
        setsid();

        execvp(program, params);
    }
    return retval;
}
#else
#include <spawn.h>

int API dw_exec(const char *program, int type, char **params)
{
    int retval = DW_ERROR_UNKNOWN;
    pid_t pid;

    /* API 28 and later has posix_spawn*() so use that instead of
     * fork/exec, however launched processes are still background
     * processes so ignore the type parameter.
     */
    if(posix_spawnp(&pid, program, NULL, NULL, params, NULL) == 0)
    {
        if(pid > 0)
            retval = pid;
        else
            retval = DW_ERROR_NONE;
    }
    return retval;
}
#endif

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 * Returns:
 *       DW_ERROR_UNKNOWN (-1) on error; DW_ERROR_NONE (0) or a positive Process ID on success.
 */
int API dw_browse(const char *url)
{
    JNIEnv *env;
    int retval = DW_ERROR_UNKNOWN;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a string
        jstring jstr = env->NewStringUTF(url);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID browseURL = env->GetMethodID(clazz, "browseURL",
                                               "(Ljava/lang/String;)I");
        // Call the method on the object
        retval = env->CallIntMethod(_dw_obj, browseURL, jstr);
        if(_dw_jni_check_exception(env))
            retval = DW_ERROR_UNKNOWN;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Creates a new print object.
 * Parameters:
 *       jobname: Name of the print job to show in the queue.
 *       flags: Flags to initially configure the print object.
 *       pages: Number of pages to print.
 *       drawfunc: The pointer to the function to be used as the callback.
 *       drawdata: User data to be passed to the handler function.
 * Returns:
 *       A handle to the print object or nullptr on failure.
 */
HPRINT API dw_print_new(const char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
    DWPrint *print;

    if(!drawfunc || !(print = (DWPrint *)calloc(1, sizeof(DWPrint))))
        return nullptr;

    if(!jobname)
        jobname = "Dynamic Windows Print Job";

    print->drawfunc = (int (*)(HPRINT, HPIXMAP, int, void *))drawfunc;
    print->drawdata = drawdata;
    print->flags = flags;
    print->jobname = strdup(jobname);
    print->pages = (jint)pages;

    return print;
}

/*
 * Runs the print job, causing the draw page callbacks to fire.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 *       flags: Flags to run the print job.
 * Returns:
 *       DW_ERROR_UNKNOWN on error or DW_ERROR_NONE on success.
 */
int API dw_print_run(HPRINT pr, unsigned long flags)
{
    JNIEnv *env;
    DWPrint *print = (DWPrint *)pr;
    int retval = DW_ERROR_UNKNOWN;

    if(print && print->drawfunc && (env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring jstr = env->NewStringUTF(print->jobname);
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID printRun = env->GetMethodID(clazz, "printRun",
                                              "(JILjava/lang/String;II)Landroid/print/PrintJob;");
        // Call the method on the object
        print->printjob = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, printRun, (jlong)pr, (jint)print->flags, jstr, (jint)print->pages, (jint)flags), _DW_REFERENCE_WEAK);
        if(print->printjob)
            retval = DW_ERROR_NONE;
        env->DeleteLocalRef(jstr);
    }
    return retval;
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT pr)
{
    JNIEnv *env;
    DWPrint *print = (DWPrint *)pr;

    if(print)
    {
        if(print->printjob && (env = (JNIEnv *) pthread_getspecific(_dw_env_key)))
        {
            // First get the class that contains the method you need to call
            jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
            // Get the method that you want to call
            jmethodID printCancel = env->GetMethodID(clazz, "printCancel",
                                                     "(Landroid/print/PrintJob;)V");
            // Call the method on the object
            env->CallVoidMethod(_dw_obj, printCancel, print->printjob);
            _dw_jni_check_exception(env);
        }
        if(print->jobname)
            free(print->jobname);
        free(print);
    }
}

/*
 * Creates a new system notification if possible.
 * Parameters:
 *         title: The short title of the notification.
 *         imagepath: Path to an image to display or nullptr if none.
 *         description: A longer description of the notification,
 *                      or nullptr if none is necessary.
 * Returns:
 *         A handle to the notification which can be used to attach a "clicked" event if desired,
 *         or nullptr if it fails or notifications are not supported by the system.
 * Remarks:
 *          This will create a system notification that will show in the notifaction panel
 *          on supported systems, which may be clicked to perform another task.
 */
HWND API dw_notification_new(const char *title, const char *imagepath, const char *description, ...)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // Construct a String
        jstring appid = env->NewStringUTF(_dw_app_id);
        jstring ntitle = env->NewStringUTF(title);
        jstring ndesc = nullptr;
        jstring image = nullptr;

        if(description)
        {
            va_list args;
            char outbuf[1025] = {0};

            va_start(args, description);
            vsnprintf(outbuf, 1024, description, args);
            va_end(args);

            ndesc = env->NewStringUTF(outbuf);
        }
        if(imagepath)
            image = env->NewStringUTF(imagepath);

        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notificationNew = env->GetMethodID(clazz, "notificationNew",
                                                     "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Landroidx/core/app/NotificationCompat$Builder;");
        // Call the method on the object
        jobject result = _dw_jni_check_result(env, env->CallObjectMethod(_dw_obj, notificationNew, ntitle, image, ndesc, appid), _DW_REFERENCE_WEAK);
        env->DeleteLocalRef(appid);
        env->DeleteLocalRef(ntitle);
        return result;
    }
    return nullptr;
}

/*
 * Sends a notification created by dw_notification_new() after attaching signal handler.
 * Parameters:
 *         notification: The handle to the notification returned by dw_notification_new().
 * Returns:
 *         DW_ERROR_NONE on success, DW_ERROR_UNKNOWN on error or not supported.
 */
int API dw_notification_send(HWND notification)
{
    JNIEnv *env;

    if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
    {
        // First get the class that contains the method you need to call
        jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
        // Get the method that you want to call
        jmethodID notificationNew = env->GetMethodID(clazz, "notificationSend",
                                                     "(Landroidx/core/app/NotificationCompat$Builder;)V");
        // Call the method on the object
        env->CallVoidMethod(_dw_obj, notificationNew, notification);
        if(!_dw_jni_check_exception(env))
            return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Converts a UTF-8 encoded string into a wide string.
 * Parameters:
 *       utf8string: UTF-8 encoded source string.
 * Returns:
 *       Wide string that needs to be freed with dw_free()
 *       or nullptr on failure.
 */
wchar_t * API dw_utf8_to_wchar(const char *utf8string)
{
    size_t buflen = strlen(utf8string) + 1;
    wchar_t *temp = (wchar_t *)malloc(buflen * sizeof(wchar_t));
    if(temp)
        mbstowcs(temp, utf8string, buflen);
    return temp;
}

/*
 * Converts a wide string into a UTF-8 encoded string.
 * Parameters:
 *       wstring: Wide source string.
 * Returns:
 *       UTF-8 encoded string that needs to be freed with dw_free()
 *       or nullptr on failure.
 */
char * API dw_wchar_to_utf8(const wchar_t *wstring)
{
    size_t bufflen = 8 * wcslen(wstring) + 1;
    char *temp = (char *)malloc(bufflen);
    if(temp)
        wcstombs(temp, wstring, bufflen);
    return temp;
}

/*
 * Gets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_FEATURE_DISABLED if the feature is supported but disabled.
 *       DW_FEATURE_ENABLED if the feature is supported and enabled.
 *       Other value greater than 1, same as enabled but with extra info.
 */
int API dw_feature_get(DWFEATURE feature)
{
    switch(feature)
    {
        case DW_FEATURE_HTML:                    /* Supports the HTML Widget */
        case DW_FEATURE_HTML_RESULT:             /* Supports the DW_SIGNAL_HTML_RESULT callback */
        case DW_FEATURE_HTML_MESSAGE:            /* Supports the DW_SIGNAL_HTML_MESSAGE callback */
        case DW_FEATURE_NOTIFICATION:            /* Supports sending system notifications */
        case DW_FEATURE_UTF8_UNICODE:            /* Supports UTF8 encoded Unicode text */
        case DW_FEATURE_MLE_WORD_WRAP:           /* Supports word wrapping in Multi-line Edit boxes */
        case DW_FEATURE_CONTAINER_STRIPE:        /* Supports striped line display in container widgets */
        case DW_FEATURE_TREE:                    /* Supports the Tree Widget */
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_CONTAINER_MODE:          /* Supports alternate container view modes */
            return _dw_container_mode;
        case DW_FEATURE_RENDER_SAFE:             /* Supports render safe drawing mode, limited to expose */
            return _dw_render_safe_mode;
        case DW_FEATURE_DARK_MODE:               /* Supports Dark Mode user interface */
        {
            /* Dark Mode on Android requires Android 10 (API 29) */
            if(_dw_android_api >= 29) {
                return _dw_dark_mode_detected();
            }
            return DW_FEATURE_UNSUPPORTED;
        }
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}

/*
 * Sets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 *       state: DW_FEATURE_DISABLED, DW_FEATURE_ENABLED or any value greater than 1.
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_ERROR_NONE if the feature is supported and successfully configured.
 *       DW_ERROR_GENERAL if the feature is supported but could not be configured.
 * Remarks:
 *       These settings are typically used during dw_init() so issue before
 *       setting up the library with dw_init().
 */
int API dw_feature_set(DWFEATURE feature, int state)
{
    switch(feature)
    {
        /* These features are supported but not configurable */
        case DW_FEATURE_HTML:                    /* Supports the HTML Widget */
        case DW_FEATURE_HTML_RESULT:             /* Supports the DW_SIGNAL_HTML_RESULT callback */
        case DW_FEATURE_HTML_MESSAGE:            /* Supports the DW_SIGNAL_HTML_MESSAGE callback */
        case DW_FEATURE_NOTIFICATION:            /* Supports sending system notifications */
        case DW_FEATURE_UTF8_UNICODE:            /* Supports UTF8 encoded Unicode text */
        case DW_FEATURE_MLE_WORD_WRAP:           /* Supports word wrapping in Multi-line Edit boxes */
        case DW_FEATURE_CONTAINER_STRIPE:        /* Supports striped line display in container widgets */
        case DW_FEATURE_TREE:                    /* Supports the Tree Widget */
            return DW_ERROR_GENERAL;
        /* These features are supported and configurable */
        case DW_FEATURE_RENDER_SAFE:             /* Supports render safe drawing mode, limited to expose */
        {
            if (state == DW_FEATURE_ENABLED || state == DW_FEATURE_DISABLED)
            {
                _dw_render_safe_mode = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
        case DW_FEATURE_CONTAINER_MODE:          /* Supports alternate container view modes */
        {
            if(state >= DW_CONTAINER_MODE_DEFAULT && state < DW_CONTAINER_MODE_MAX)
            {
                JNIEnv *env;

                if((env = (JNIEnv *)pthread_getspecific(_dw_env_key)))
                {
                    // First get the class that contains the method you need to call
                    jclass clazz = _dw_find_class(env, DW_CLASS_NAME);
                    // Get the method that you want to call
                    jmethodID setContainerMode = env->GetMethodID(clazz, "setContainerMode",
                                                                  "(I)V");
                    _dw_container_mode = state;
                    // Call the method on the object
                    env->CallVoidMethod(_dw_obj, setContainerMode, (jint)state);
                    _dw_jni_check_exception(env);
                }
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
        case DW_FEATURE_DARK_MODE:               /* Supports Dark Mode user interface */
        {
            /* Dark Mode on Android requires 10 (API 29) */
            if(_dw_android_api >= 29) {
                /* While technically configurable....
                 * for now just return DW_ERROR_GENERAL
                 */
                return DW_ERROR_GENERAL;
            }
            return DW_FEATURE_UNSUPPORTED;
        }
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}

#ifdef __cplusplus
}
#endif
