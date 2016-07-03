# Tools to interface with Duet

The following tools are included in the repo:
* Duet library (libduet) for user-space applications
* Duet command line tools to interact with module
* Example duet-enabled application
* Setup script to compile and install dependencies, kernel, module, tools


## KVM kernel debugging guide

The following steps are meant to guide you through setting up a KVM server,
compiling the Duet kernel on the host, installing and running a Duet-enabled
kernel within a VM, and debugging said kernel from the host. If you already
have an existing KVM server, skip to step 3.

1. Install [KVM](https://help.ubuntu.com/community/KVM/Installation).

   ```bash
   # Install Ubuntu dependencies
   sudo apt install qemu-kvm libvirt-bin ubuntu-vm-builder bridge-utils
   
   # Add yourself (and others) to groups
   sudo adduser `id -un` kvm
   sudo adduser `id -un` libvirtd
   ```

2. To get a web interface to QEMU/KVM, install Kimchi.

   ```bash
   # Download Wok and plugins
   wget http://kimchi-project.github.io/wok/downloads/wok-2.2.0-0.noarch.deb
   wget http://kimchi-project.github.io/gingerbase/downloads/ginger-base-2.2.0-0.noarch.deb
   wget http://kimchi-project.github.io/kimchi/downloads/kimchi-2.2.0-0.noarch.deb
   
   # Install deb packages. Dependency issues may occur, so we'll handle those too.
   dpkg -i wok-2.2.0-0.noarch.deb
   sudo apt -f install
   dpkg -i ginger-base-2.2.0-0.noarch.deb
   sudo apt -f install
   dpkg -i kimchi-2.2.0-0.noarch.deb
   sudo apt -f install
   ```
   
   You can now access the Wok interface at https://localhost:8001. If you need
   to forward the port, do:
   
   ```bash
   ssh -f -N -L 8001:wokserver:8001 user@gateway
   ```

3. Clone the duet repos within the **same** directory at the **host**. Let's
   assume you are cloning them under */media/data/duet*:

   ```bash
   # Get the sources
   cd /media/data/duet
   git clone git@github.com:gamvrosi/duet-kernel.git
   git clone git@github.com:gamvrosi/duet-module.git
   git clone git@github.com:gamvrosi/duet-tools.git
   
   # Switch duet-kernel to branch with duet hook support
   cd duet-kernel
   git checkout duet
   ```

4. Create a new VM under KVM, running Ubuntu 16.04 server. Edit the VM's config
   to enable the QEMU/KVM stub. Assuming your VM's name in KVM is
   ```duetdev```, run:

   ```bash
   $ virsh edit duetdev
   ```

   When editing, replace the ```<domain type='kvm'>``` line with:

   ```xml
   <domain type='kvm' xmlns:qemu='http://libvirt.org/schemas/domain/qemu/1.0'>
   <qemu:commandline>
     <qemu:arg value='-gdb'/>
     <qemu:arg value='tcp::1234'/>
   </qemu:commandline>
   ```

   This config file is generally found at */etc/libvirt/qemu/*, although you
   shouldn't edit it [manually](https://gymnasmata.wordpress.com/2010/12/02/setting-up-gdb-to-work-with-qemu-kvm-via-libvirt/).
   Finally, if you are running multiple VMs on the same host, and want to hook
   into more than one of them, make sure to pick a unique TCP port number for
   each!

5. Get NFS server up and running on the host:

   ```bash
   sudo apt install nfs-kernel-server
   sudo mkdir -p /export/duet
   ```

   Update ```/etc/fstab```:

   ```
   /media/data/duet	/export/duet	none	bind	0	0
   ```

   Update ```/etc/exports```. Assumimg your guest's IP is in ```192.168.122.0/24```, add:

   ```
   /export		192.168.122.0/24(rw,fsid=0,insecure,no_subtree_check,async)
   /export/duet	192.168.122.0/24(rw,nohide,insecure,no_subtree_check,async)
   ```

   Then export the mount points:

   ```bash
   sudo mount -a
   sudo exportfs -a
   ```

6. Get NFS client up and running on the KVM guest:

   ```bash
   sudo apt install nfs-common
   ```

   Update ```/etc/fstab```. Assuming your host's virbr0 IP is ```192.168.122.1```, do:

   ```
   192.168.122.1:/duet		/media/duet	nfs	auto	0	0
   ```

   Create mount point and mount export:

   ```bash
   sudo mkdir /media/duet
   sudo mount -a
   ```

7. Install dependencies needed for compilation **at the host and the guest**:

   ```bash
   cd duet-tools
   ./setup.sh -d
   ```

8. Configure the kernel at the **guest** (be mindful of NFS ID mapping):

   ```bash
   ./setup.sh -c
   ```

9. Compile kernel at the **host**:

   ```bash
   ./setup.sh -k
   ```

10. Install kernel at the **guest**:

    ```bash
    ./setup.sh -K
    ```

11. (*Optional*) Create a ```~/.gdbinit``` file at the host, containing:

    ```
    set auto-load safe-path /
    ```

    This will override gdb's security protection and allow ```vmlinux-gdb.py``` to
    execute. Or add something more responsible, like:

    ```
    add-auto-load-safe-path /media/data/duet/duet-kernel/scripts/gdb/vmlinux-gdb.py
    ```

12. Start gdb on host

    ```
    $ cd /media/data/duet/duet-kernel
    $ gdb vmlinux
    (gdb) target remote :1234
    ```

13. Compile, install, and load module on guest

    ```
    $ ./setup.sh -m && ./setup.sh -M
    ```

14. Compile, install, and load tools on guest

    ```
    $ ./setup.sh -t && ./setup.sh -T
    ```

### Parting tips and tricks

 * If you want to recompile, do so on the host. Then follow guide starting at
   step 9.
 * On Ubuntu you can change hostname by editing ```/etc/hostname```,
   ```/etc/hosts```, and running ```sudo service hostname restart```.
 * The dummy task is your friend in understanding how the Duet API works.

### [Examples](https://www.kernel.org/doc/Documentation/gdb-kernel-debugging.txt) of using the Linux-provided gdb helpers

- Load module (and main kernel) symbols:

  ```
   (gdb) lx-symbols
   loading vmlinux
   scanning for modules in /home/user/linux/build
   loading @0xffffffffa0020000: /home/user/linux/build/net/netfilter/xt_tcpudp.ko
   loading @0xffffffffa0016000: /home/user/linux/build/net/netfilter/xt_pkttype.ko
   loading @0xffffffffa0002000: /home/user/linux/build/net/netfilter/xt_limit.ko
   loading @0xffffffffa00ca000: /home/user/linux/build/net/packet/af_packet.ko
   loading @0xffffffffa003c000: /home/user/linux/build/fs/fuse/fuse.ko
   ...
   loading @0xffffffffa0000000: /home/user/linux/build/drivers/ata/ata_generic.ko
  ```

- Set a breakpoint on some not yet loaded module function, e.g.:

  ```
   (gdb) b btrfs_init_sysfs
   Function "btrfs_init_sysfs" not defined.
   Make breakpoint pending on future shared library load? (y or [n]) y
   Breakpoint 1 (btrfs_init_sysfs) pending.
  ```

- Continue the target

  ```
   (gdb) c
  ```

- Load the module on the target and watch the symbols being loaded as well as the breakpoint hit:

  ```
   loading @0xffffffffa0034000: /home/user/linux/build/lib/libcrc32c.ko
   loading @0xffffffffa0050000: /home/user/linux/build/lib/lzo/lzo_compress.ko
   loading @0xffffffffa006e000: /home/user/linux/build/lib/zlib_deflate/zlib_deflate.ko
   loading @0xffffffffa01b1000: /home/user/linux/build/fs/btrfs/btrfs.ko

   Breakpoint 1, btrfs_init_sysfs () at /home/user/linux/fs/btrfs/sysfs.c:36
   36              btrfs_kset = kset_create_and_add("btrfs", NULL, fs_kobj);
  ```

- Dump the log buffer of the target kernel:

  ```
   (gdb) lx-dmesg
   [     0.000000 ] Initializing cgroup subsys cpuset
   [     0.000000 ] Initializing cgroup subsys cpu
   [     0.000000 ] Linux version 3.8.0-rc4-dbg+ (...
   [     0.000000 ] Command line: root=/dev/sda2 resume=/dev/sda1 vga=0x314
   [     0.000000 ] e820: BIOS-provided physical RAM map:
   [     0.000000 ] BIOS-e820: [mem 0x0000000000000000-0x000000000009fbff] usable
   [     0.000000 ] BIOS-e820: [mem 0x000000000009fc00-0x000000000009ffff] reserved
   ....
  ```

- Examine fields of the current task struct:

  ```
   (gdb) p $lx_current().pid
   $1 = 4998
   (gdb) p $lx_current().comm
   $2 = "modprobe\000\000\000\000\000\000\000"
  ```

- Make use of the per-cpu function for the current or a specified CPU:

  ```
   (gdb) p $lx_per_cpu("runqueues").nr_running
   $3 = 1
   (gdb) p $lx_per_cpu("runqueues", 2).nr_running
   $4 = 0
  ```

- Dig into hrtimers using the container_of helper:

  ```
   (gdb) set $next = $lx_per_cpu("hrtimer_bases").clock_base[0].active.next
   (gdb) p *$container_of($next, "struct hrtimer", "node")
	$5 = {
		node = {
			node = {
         __rb_parent_color = 18446612133355256072,
         rb_right = 0x0 <irq_stack_union>,
         rb_left = 0x0 <irq_stack_union>
       
			},
			expires = {
         tv64 = 1835268000000
       
			}
     
		},
		_softexpires = {
       tv64 = 1835268000000
     
		},
     function = 0xffffffff81078232 <tick_sched_timer>,
     base = 0xffff88003fd0d6f0,
     state = 1,
     start_pid = 0,
     start_site = 0xffffffff81055c1f <hrtimer_start_range_ns+20>,
     start_comm = "swapper/2\000\000\000\000\000\000"
   
	}
  ```

- Dig into a radix tree data structure, such as the IRQ descriptors:

  ```
   (gdb) print (struct irq_desc)$lx_radix_tree_lookup(irq_desc_tree, 18)
	$6 = {
		irq_common_data = {
       state_use_accessors = 67584,
       handler_data = 0x0 <__vectors_start>,
       msi_desc = 0x0 <__vectors_start>,
       affinity = {{
           bits = {65535}
         
       }}
     
		},
		irq_data = {
       mask = 0,
       irq = 18,
       hwirq = 27,
       common = 0xee803d80,
       chip = 0xc0eb0854 <gic_data>,
       domain = 0xee808000,
       parent_data = 0x0 <__vectors_start>,
       chip_data = 0xc0eb0854 <gic_data>
     
		} <... trimmed ...>
  ```

### List of commands and functions

The number of commands and convenience functions may evolve over the time, this
is just a snapshot of the initial version:

```
 (gdb) apropos lx
 function lx_current -- Return current task
 function lx_module -- Find module by name and return the module variable
 function lx_per_cpu -- Return per-cpu variable
 function lx_task_by_pid -- Find Linux task by PID and return the task_struct variable
 function lx_thread_info -- Calculate Linux thread_info from task variable
 lx-dmesg -- Print Linux kernel log buffer
 lx-lsmod -- List currently loaded modules
 lx-symbols -- (Re-)load symbols of Linux kernel and currently loaded modules
```

Detailed help can be obtained via ```help <command-name>``` for commands and
```help function <function-name>``` for convenience functions.


## Using perf with Duet

_perf_ is a powerful tool for profiling, tracing and instrumenting the Linux
kernel, and is extremely useful for diagnosing performance issues that cross
the userspace/kernel system call boundary. In order to use perf with the Duet
kernel, include the following options in your build configuration:

```
 CONFIG_KPROBES=y
 CONFIG_HAVE_KPROBES=y
 CONFIG_KPROBES_ON_FTRACE=y
 CONFIG_KPROBE_EVENT=y
 CONFIG_UPROBES=y
 CONFIG_UPROBE_EVENT=y
 CONFIG_FTRACE=y
 CONFIG_FTRACE_SYSCALLS=y
 CONFIG_DYNAMIC_FTRACE=y
 CONFIG_FUNCTION_PROFILER=y
 CONFIG_FUNCTION_TRACER=y
 CONFIG_FUNCTION_GRAPH_TRACR=y
 CONFIG_STACKTRACE_SUPPORT=y
 CONFIG_TRACEPOINTS=y
 CONFIG_PERF_EVENTS=y
```

Also, `CONFIG_DEBUG_INFO=y` and `CONFIG_DEBUG_KERNEL=y` are a good idea too
(be sure to run `./setup.sh -g` and install the resulting
`linux-image-3.13.6+duet-$SHA1-dbg` package in order to give perf the
necessary debug info found under
`/usr/lib/debug/lib/modules/3.13.6+duet-$SHA1/vmlinux`.)

Unfortunately, the userspace tool found in `linux-3.13.6+duet/tools/perf` is
unusable. Instead, on Debian and Ubuntu systems, install the
`linux-tools-X.Y.Z-W` package, where the kernel version _X.Y.Z-W_ is whichever
is the highest available as reported by `apt-cache search linux-tools` (at the
time of this writing, on Ubuntu 15.10 "Wily" this is in the 4.2.0 range.)

Next, as `root` run the `perf` binary found under
`/usr/lib/linux-tools-X.Y.Z-W/perf`. If you see a message such as _WARNING:
perf not found for kernel 3.13.6+duet_, you have actually only run the shell
script frontend `/usr/bin/perf` installed by the `linux-tools-common` package,
and not the real binary.

### A few basic perf commands

 * `perf top`: see which kernel and userspace process functions are consuming
   the most CPU
 * `perf top --call-graph dwarf,1024`: see which functions are consuming CPU
   along with the graph of callsites that invoke them, broken down by CPU
   usage
 * `perf trace -p $PID1,$PID2,...`: trace system calls and events in the
   spirit of _strace_ but with much less performance impact on the traced
   process

### Useful perf links

 * <https://perf.wiki.kernel.org/index.php/Main_Page>
 * <https://perf.wiki.kernel.org/index.php/Tutorial>
 * <http://www.brendangregg.com/perf.html>

