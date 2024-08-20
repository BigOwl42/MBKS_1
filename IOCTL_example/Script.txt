C:
cd C:\Windows\System32\drivers
sc stop MyFilter
sc delete MyFilter
del ProcCreateFilter.sys
copy C:\Users\User\Desktop\MBKS\Laba1\ProcCreateFilter\x64\Release\ProcCreateFilter.sys ProcCreateFilter.sys 
sc create MyFilter type=kernel binPath=C:\Windows\System32\drivers\ProcCreateFilter.sys
sc start MyFilter