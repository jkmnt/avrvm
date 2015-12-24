avrvm
=====
avrvm is an embeddable virtual machine for executing AVR MCU instructions.

Sometimes applications require user scripting. The common solution is to embed a scripting language (Python, Ruby, Lua, Javascript) running in a safe sandboxed environment and to provide a host API for interaction with application.
Small embedded projects may benefit from scripting too, but limited resources prevent us from using a full-scale scripting engine.

User script may be interpreted in runtime or compiled to bytecode for execution in virtual machine.
Most scripting languages are compiled to bytecode since running bytecode in VM is a way faster than interpreting.
When designing scripting language, one should decide on VM type (stack or register based), instruction set, language grammar and features. Tools such as a compiler should be created too. Too much trouble for a small scripting needs !

What if we could use an existing and proven VM instruction set, free tools and familiar language ?
Enter avrvm :-) Instruction set is same as in AVR MCUs, familiar to oldtime embedded engineers and arduino kiddies alike.
Language is C, assembly or anything compilable to AVR binaries. In case of C, AVR-GCC may be used for script compiling.

Features
--------
* avrvm have no external dependencies
* avrvm API is just two functions - avrvm_init and avrvm_exec plus a few callbacks for interfacing with memory
* bytecode may be fetched from any media with random access (internal flash, ram, sdcard, you name it)
* easy multitasking - script may be paused on calling async host function and resumed upon receiving response some time later. For the script, the call will look like a plain blocking call.
* several instances of VMs may be running in parallel, cooperatively sharing time if control is passed between VMs in a round-robin way.
* script may complete and 'exit' via 'break' assembly instruction

Limitations
-----------
* no AVR peripheral emulation at all - it's not intended to mimic hardware.
* no emulation of instructions timing for the same reason.
* no interrupts - scripts need no stinky interrupts !

Host interface
--------------
Host API functions are presented to script as a 'ROM-ed' function table at addresses above 32k.
VM will recognize calls to these addresses and treat them in a special way, reporting function table index to host.
Another way of passing data to/from script is to use a 61 I/O registers.

VM API
---
VM instance is initialized via call to avrvm_init.

    static avrvm_ctx_t vm;
    avrvm_iface_t iface =
    {
      .flash_r = flash_reader,
      .sram_r = sram_reader,
      .sram_w = sram_writer,
      .io_r = io_reader,
      .io_w = io_writer,
    };
    avrvm_init(&vm, &iface, sizeof(sram));

Script is executed by repeatedly calling avrvm_exec. Single instruction is executed.

    int rc = avrvm_exec(&vm);

Negative return value maps to

    enum avrvm_rc_e
    {
        AVRVM_RC_UNDEF_INSTR = -5,
        AVRVM_RC_WDR = -4,
        AVRVM_RC_BREAK = -3,
        AVRVM_RC_SLEEP = -2,
        AVRVM_RC_OK = -1,
    };

, while non-negative value should be treated as a call of host function from 'ROM' table with index == rc.

What’s next ?
-----------
See project in /hello for example of usage
