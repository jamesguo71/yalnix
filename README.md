# Yalnix - Team Unics
This repository contains our (Fei & Taylor) code for the COSC258 Yalnix project.

# Checkpoint 4
Implement the following functions

```bash
Fork
Exec
Exit
Wait
TrapKernel
TrapIllegal
TrapMemory
TrapMath
TrapTTYRreceive
TrapTTYWrite
TrapDisk
```

* Add SchedulerUpdateTerminated which takes the parent pcb and loops over its children. For any children that have exited, it removes their pcb from the terminated list and frees it. We need to do this, otherwise a parent may exit and all of its children will add themselves to the terminated list never to be removed.