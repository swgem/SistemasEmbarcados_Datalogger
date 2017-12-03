#include "cmsis_os.h"
 
void Thread (void const *arg)                             // function prototype for a Thread
{
  ...
}
osThreadDef (Thread, osPriorityNormal, 3, 0);              // define Thread and specify to allow three instances
 
void ThreadCreate_example (void) {
  osThreadId id1, id2, id3;
  
  id1 = osThreadCreate (osThread (Thread), NULL);          // create the thread with id1

  
  if (id1 == NULL) {                                       // handle thread creation for id1
    // Failed to create the thread with id1
  }
  :
  osThreadTerminate (id1);                                  // stop the thread with id1

}