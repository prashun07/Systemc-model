
# Writing Your Own Startup Code for Cortex-M (Easy Summary)

## What is startup code?

Startup code is the **very first program executed after a Cortex-M processor comes out of reset**.

Its job is to prepare the microcontroller so that a C/C++ application can run correctly.

Think of it like preparing a classroom before students arrive:

* Arrange the desks
* Turn on the lights
* Put books in place

Only then can the class begin.

Similarly, startup code prepares memory and the processor before calling your `main()` function. ([Arm Developer][1])

---

# What happens after Reset?

When the Cortex-M resets:

1. The processor reads the **initial stack pointer** from the first entry of the vector table.
2. It reads the **Reset Handler address** from the second entry.
3. It sets the Stack Pointer.
4. It jumps to the Reset Handler.

The Reset Handler is where the startup code begins executing. ([Arm Developer][1])

---

# What is the Vector Table?

The vector table is simply an **array of addresses** stored in Flash memory.

It contains:

* Initial Stack Pointer
* Reset Handler
* Addresses of all exception handlers
* Addresses of interrupt handlers

Conceptually:

```
Vector Table

+------------------------+
| Initial Stack Pointer  |
+------------------------+
| Reset_Handler          |
+------------------------+
| NMI_Handler            |
+------------------------+
| HardFault_Handler      |
+------------------------+
| MemManage_Handler      |
+------------------------+
| BusFault_Handler       |
+------------------------+
| UsageFault_Handler     |
+------------------------+
| ...                    |
+------------------------+
| Peripheral IRQs        |
+------------------------+
```

Whenever an interrupt occurs, the processor looks up the corresponding address in this table and jumps to that handler. ([Arm Developer][1])

---

# What does the Reset Handler do?

The article explains that the Reset Handler typically performs the following tasks:

## 1. Set up the Stack Pointer

The processor loads the initial stack pointer from the vector table before executing the Reset Handler.

This gives the program a valid stack for function calls and local variables. ([Arm Developer][1])

---

## 2. Initialize RAM

Global variables are divided into two groups.

### Initialized variables (.data)

Example:

```c
int counter = 10;
```

The initial value (`10`) is stored in Flash.

At startup:

* Copy from Flash
* Place into RAM

```
Flash
------
counter = 10

↓

RAM
------
counter = 10
```

([Arm Developer][1])

---

### Uninitialized variables (.bss)

Example:

```c
int count;
```

These have no stored initial value.

The startup code clears this memory by setting it to zero.

```
Before

RAM
---------
count = ?

↓

After

RAM
---------
count = 0
```

([Arm Developer][1])

---

## 3. Call `main()`

Once memory has been initialized, the startup code calls:

```c
main();
```

From this point onward, the application executes normally. ([Arm Developer][1])

---

# Why is assembly language used?

The article explains that startup code is usually written in assembly because:

* It executes before the C runtime is initialized.
* The stack and memory are not yet prepared.
* The programmer needs direct control over processor initialization.

Assembly allows these low-level operations to be performed immediately after reset. ([Arm Developer][1])

---

# What is the purpose of the assembler directives?

The tutorial introduces GNU assembler (GAS) directives that organize the program rather than generate CPU instructions.

Examples discussed include directives used to:

* Define sections
* Export symbols
* Specify function types
* Align data
* Place the vector table and code correctly in memory

These directives help the linker place different parts of the program in the appropriate memory locations. ([Arm Developer][1])

---

# Weak interrupt handlers

The article shows that interrupt handlers are often declared as **weak symbols**.

This allows the startup file to provide default handlers while enabling the application to replace only the handlers it needs.

For example:

```
Default_Handler

↓

User defines

UART_Handler()

↓

UART interrupt now jumps to UART_Handler
```

The user does not need to modify the startup file itself. ([Arm Developer][1])

---

# What happens if an interrupt is not implemented?

If an interrupt occurs and no custom handler has been provided, execution goes to the default handler supplied by the startup code.

This makes it easy to detect unexpected interrupts during debugging. ([Arm Developer][1])

---

# Overall startup flow

```text
Power On / Reset
        │
        ▼
Read Initial Stack Pointer
        │
        ▼
Read Reset_Handler Address
        │
        ▼
Execute Reset_Handler
        │
        ▼
Copy .data (Flash → RAM)
        │
        ▼
Zero .bss
        │
        ▼
Call main()
        │
        ▼
Application Starts Running
```

---

# Key takeaways

* Startup code is the first code executed after a Cortex-M reset.
* The processor obtains the **initial stack pointer** and **Reset Handler** from the vector table.
* The vector table stores the addresses of exception and interrupt handlers.
* The Reset Handler prepares memory by copying initialized data to RAM and clearing uninitialized data.
* After initialization, it calls `main()`.
* Startup code is typically written in assembly because it runs before the C runtime environment is ready.
* Weak default interrupt handlers let applications override only the handlers they require. ([Arm Developer][1])

[1]: https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/writing-your-own-startup-code-for-cortex-m?utm_source=chatgpt.com "Writing your own startup code for Cortex-M"
