[Defines]
  PLATFORM_NAME           = 뇌절OS
  PLATFORM_GUID           = 66666666-7777-8888-9999-AAAAAAAAAAAA
  DSC_SPECIFICATION       = 0x00010006
  OUTPUT_DIRECTORY        = Build/뇌절OS
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = RELEASE

[LibraryClasses]
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

[Components]
  뇌절OS_App.inf
