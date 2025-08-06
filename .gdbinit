file bin/os.bin
set architecture i386
set disassembly-flavor intel
set breakpoint pending on

# Define a hook to run after 'target remote' connects
define hookpost-target
  # Breakpoints go here (now that target is connected and symbols loaded)
  break *0x0010000c
  # break _start
  break kernel_main
  # continue
end

define rq 
  # Disconnect from current target
  disconnect

  # Kill existing QEMU (assumes you know the PID or use pkill; adjust for your OS)
  shell pkill -f "qemu-system-i386"  

  # Relaunch QEMU in background (adjust path/command)
  shell qemu-system-i386 -kernel bin/os.bin -s -S &  

  # Brief delay for QEMU to start listening
  shell sleep 1

  # Reconnect
  target remote localhost:1234

  hookpost-target
end

define reload-os
  file bin/os.bin
  # Add-symbol-file commands as above if needed
  echo Symbols reloaded!\n
end

target remote localhost:1234

hookpost-target

