# Cap'n Hook
*A hook manager for the TI-84 Plus CE and TI-83 Premium CE.*

[Hooks](https://wikiti.brandonw.net/index.php?title=83Plus:OS:Hooks) are a feature of TI-OS that allow apps and programs to run their own code whenever an a event occurs in the OS - for example, using a token hook, you can change how tokens are displayed in TI-OS.

A major limitation of hooks is that only one program can install a hook of a particular type. If another program wants to use the same type of hook, it must either prevent to first hook from using it, or implement chaining logic.

Cap'n Hook seeks to prevent this problem by providing a common interface for programs to register their hooks with. This has a number of advantages over the OS's interface:
* Multiple programs can use the same type of hook.
* Individual programs do not have to implement chaining logic.
* If a hook is changed by a program that does not use Cap'n Hook, the next time a program that does use Cap'n Hook is run, the old hooks are restored.
* In case two hooks conflict with each other (for example, two programs both want to open a menu when a certain key is pressed), the user can assign them a priority to change which one will run.

Cap'n Hook does not currently provide a mechanism for storing hooks persistently. Hooks should be stored somewhere that is unlikely to be overwritten - for example, in a deleted appvar, which will persist until the next garbage collect.
