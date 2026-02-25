# 遇到常见的编译失败的问题

```sh
      |                                ^~~~~~~~~~~~~~~~~~~
/usr/local/include/c++/12.1.0/ext/concurrence.h:252:32: 错误：cannot convert ‘<花括号内的初始值列表>’ to ‘unsigned int’ in initialization
  252 |     __gthread_cond_t _M_cond = __GTHREAD_COND_INIT;
      |                                ^~~~~~~~~~~~~~~~~~~
make[2]: *** [CMakeFiles/tinycoro.dir/build.make:191：CMakeFiles/tinycoro.dir/src/io/io_awaiter.cpp.o] 错误 1
In file included from /usr/local/include/c++/12.1.0/x86_64-pc-linux-gnu/bits/gthr-default.h:35,
                 from /usr/local/include/c++/12.1.0/x86_64-pc-linux-gnu/bits/gthr.h:148,
                 from /usr/local/include/c++/12.1.0/bits/atomic_wait.h:38,
                 from /usr/local/include/c++/12.1.0/bits/atomic_base.h:41,
                 from /usr/local/include/c++/12.1.0/atomic:41,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/include/coro/comp/mutex.hpp:13,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/include/coro/comp/condition_variable.hpp:16,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/src/comp/condition_variable.cpp:1:
/usr/local/include/c++/12.1.0/bits/std_mutex.h:190:32: 错误：cannot convert ‘<花括号内的初始值 列表>’ to ‘unsigned int’ in initialization
  190 |     __gthread_cond_t _M_cond = __GTHREAD_COND_INIT;
      |                                ^~~~~~~~~~~~~~~~~~~
/usr/local/include/c++/12.1.0/ext/concurrence.h:252:32: 错误：cannot convert ‘<花括号内的初始值列表>’ to ‘unsigned int’ in initialization
  252 |     __gthread_cond_t _M_cond = __GTHREAD_COND_INIT;
      |                                ^~~~~~~~~~~~~~~~~~~
make[2]: *** [CMakeFiles/tinycoro.dir/build.make:93：CMakeFiles/tinycoro.dir/src/comp/condition_variable.cpp.o] 错误 1
In file included from /usr/local/include/c++/12.1.0/x86_64-pc-linux-gnu/bits/gthr-default.h:35,
                 from /usr/local/include/c++/12.1.0/x86_64-pc-linux-gnu/bits/gthr.h:148,
                 from /usr/local/include/c++/12.1.0/bits/atomic_wait.h:38,
                 from /usr/local/include/c++/12.1.0/bits/atomic_base.h:41,
                 from /usr/local/include/c++/12.1.0/atomic:41,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/include/coro/engine.hpp:14,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/include/coro/io/base_io_type.hpp:1,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/include/coro/io/net/tcp/tcp.hpp:13,
                 from /mnt/hdd/users/filament/Courses/tinyCoroLab/src/io/net/tcp/tcp.cpp:3:
```

然后查了一下上面的cmake输出
```sh
filament@pink:~/Courses/tinyCoroLab/build$ cmake ..
-- The CXX compiler identification is GNU 12.1.0
-- The C compiler identification is GNU 15.2.1
```

发现这里的libc++是gcc12, 但是c是gcc15

我们查一下版本, 发现是用户和系统自带的出了问题, 而且这个我们应该使用高版本的也就是gcc15
```sh
filament@pink:~/Courses/tinyCoroLab/note$ /usr/bin/g++ --version
g++ (GCC) 15.2.1 20251211 (Red Hat 15.2.1-5)
Copyright © 2025 Free Software Foundation, Inc.
本程序是自由软件；请参看源代码的版权声明。本软件没有任何担保；
包括没有适销性和某一专用目的下的适用性担保。
filament@pink:~/Courses/tinyCoroLab/note$ /usr/bin/gcc --version
gcc (GCC) 15.2.1 20251211 (Red Hat 15.2.1-5)
Copyright © 2025 Free Software Foundation, Inc.
本程序是自由软件；请参看源代码的版权声明。本软件没有任何担保；
包括没有适销性和某一专用目的下的适用性担保。
filament@pink:~/Courses/tinyCoroLab/note$ g++ --version
g++ (GCC) 12.1.0
Copyright © 2022 Free Software Foundation, Inc.
本程序是自由软件；请参看源代码的版权声明。本软件没有任何担保；
包括没有适销性和某一专用目的下的适用性担保。
filament@pink:~/Courses/tinyCoroLab/note$ gcc --version
gcc (GCC) 12.1.0
Copyright © 2022 Free Software Foundation, Inc.
本程序是自由软件；请参看源代码的版权声明。本软件没有任何担保；
包括没有适销性和某一专用目的下的适用性担保。
```

那么我们在cmake调整参数就行了

```sh
# 在build/ 下
rm -rf *

cmake .. \
  -DCMAKE_C_COMPILER=/usr/bin/gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++

make -j$(nproc)
```


输出如下
```sh
filament@pink:~/Courses/tinyCoroLab/build$ cmake .. \
  -DCMAKE_C_COMPILER=/usr/bin/gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++

-- The CXX compiler identification is GNU 15.2.1
-- The C compiler identification is GNU 15.2.1
-- Generating done (0.2s)
-- Build files have been written to: /mnt/hdd/users/filament/Courses/tinyCoroLab/build
```

make

```
filament@pink:~/Courses/tinyCoroLab/build$ make -j$(nproc)
[100%] Linking CXX executable ../bin/stack_call
[100%] Built target stack_call
```
