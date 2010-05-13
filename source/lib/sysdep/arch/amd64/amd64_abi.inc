%ifdef OS_UNIX
%define arg0 rdi
%define arg1 rsi
%define arg2 rdx
%define arg3 rcx
%else
%define arg0 rcx
%define arg1 rdx
%define arg2 r8
%define arg3 r9
%endif

