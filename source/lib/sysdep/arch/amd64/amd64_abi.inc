%ifdef OS_WIN
%define arg0 rcx
%define arg1 rdx
%define arg2 r8
%define arg3 r9
%else
%define arg0 rdi
%define arg1 rsi
%define arg2 rdx
%define arg3 rcx
%endif

