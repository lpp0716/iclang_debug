#include "illvm/FuncV/ELF/Type.h"

namespace illvm {
namespace funcv {
namespace elf {

std::string TypeToString::elfMachineToString(const uint16_t machine) {
  // http://www.uxsglobal.com/developers/gabi/latest/ch4.eheader.html
  switch (machine) {
  case llvm::ELF::EM_NONE:
    return "No machine";
  case llvm::ELF::EM_M32:
    return "AT&T WE 32100";
  case llvm::ELF::EM_SPARC:
    return "SPARC";
  case llvm::ELF::EM_386:
    return "Intel 386";
  case llvm::ELF::EM_68K:
    return "Motorola 68000";
  case llvm::ELF::EM_88K:
    return "Motorola 88000";
  case llvm::ELF::EM_IAMCU:
    return "Intel MCU";
  case llvm::ELF::EM_860:
    return "Intel 80860";
  case llvm::ELF::EM_MIPS:
    return "MIPS R3000";
  case llvm::ELF::EM_S370:
    return "IBM System/370";
  case llvm::ELF::EM_MIPS_RS3_LE:
    return "MIPS RS3000 Little-endian";
  case llvm::ELF::EM_PARISC:
    return "Hewlett-Packard PA-RISC";
  case llvm::ELF::EM_VPP500:
    return "Fujitsu VPP500";
  case llvm::ELF::EM_SPARC32PLUS:
    return "Enhanced instruction set SPARC";
  case llvm::ELF::EM_960:
    return "Intel 80960";
  case llvm::ELF::EM_PPC:
    return "PowerPC";
  case llvm::ELF::EM_PPC64:
    return "PowerPC64";
  case llvm::ELF::EM_S390:
    return "IBM System/390";
  case llvm::ELF::EM_SPU:
    return "IBM SPU/SPC";
  case llvm::ELF::EM_V800:
    return "NEC V800";
  case llvm::ELF::EM_FR20:
    return "Fujitsu FR20";
  case llvm::ELF::EM_RH32:
    return "TRW RH-32";
  case llvm::ELF::EM_RCE:
    return "Motorola RCE";
  case llvm::ELF::EM_ARM:
    return "ARM";
  case llvm::ELF::EM_ALPHA:
    return "DEC Alpha";
  case llvm::ELF::EM_SH:
    return "Hitachi SH";
  case llvm::ELF::EM_SPARCV9:
    return "SPARC V9";
  case llvm::ELF::EM_TRICORE:
    return "Siemens TriCore";
  case llvm::ELF::EM_ARC:
    return "Argonaut RISC Core";
  case llvm::ELF::EM_H8_300:
    return "Hitachi H8/300";
  case llvm::ELF::EM_H8_300H:
    return "Hitachi H8/300H";
  case llvm::ELF::EM_H8S:
    return "Hitachi H8S";
  case llvm::ELF::EM_H8_500:
    return "Hitachi H8/500";
  case llvm::ELF::EM_IA_64:
    return "Intel IA-64 processor architecture";
  case llvm::ELF::EM_MIPS_X:
    return "Stanford MIPS-X";
  case llvm::ELF::EM_COLDFIRE:
    return "Motorola ColdFire";
  case llvm::ELF::EM_68HC12:
    return "Motorola M68HC12;";
  case llvm::ELF::EM_MMA:
    return "Fujitsu MMA Multimedia Accelerator";
  case llvm::ELF::EM_PCP:
    return "Siemens PCP";
  case llvm::ELF::EM_NCPU:
    return "Sony nCPU embedded RISC processor";
  case llvm::ELF::EM_NDR1:
    return "Denso NDR1 microprocessor";
  case llvm::ELF::EM_STARCORE:
    return "Motorola Star*Core processor";
  case llvm::ELF::EM_ME16:
    return "Toyota ME16 processor";
  case llvm::ELF::EM_ST100:
    return "STMicroelectronics ST100 processor";
  case llvm::ELF::EM_TINYJ:
    return "Advanced Logic Corp. TinyJ embedded processor family";
  case llvm::ELF::EM_X86_64:
    return "AMD x86-64 architecture";
  case llvm::ELF::EM_PDSP:
    return "Sony DSP Processor";
  case llvm::ELF::EM_PDP10:
    return "Digital Equipment Corp. PDP-10";
  case llvm::ELF::EM_PDP11:
    return "Digital Equipment Corp. PDP-11";
  case llvm::ELF::EM_FX66:
    return "Siemens FX66 microcontroller";
  case llvm::ELF::EM_ST9PLUS:
    return "STMicroelectronics ST9+ 8/16 bit microcontroller";
  case llvm::ELF::EM_ST7:
    return "STMicroelectronics ST7 8-bit microcontroller";
  case llvm::ELF::EM_68HC16:
    return "Motorola MC68HC16 Microcontroller";
  case llvm::ELF::EM_68HC11:
    return "Motorola MC68HC11 Microcontroller";
  case llvm::ELF::EM_68HC08:
    return "Motorola MC68HC08 Microcontroller";
  case llvm::ELF::EM_68HC05:
    return "Motorola MC68HC05 Microcontroller";
  case llvm::ELF::EM_SVX:
    return "Silicon Graphics SVx";
  case llvm::ELF::EM_ST19:
    return "STMicroelectronics ST19 8-bit microcontroller";
  case llvm::ELF::EM_VAX:
    return "Digital VAX";
  case llvm::ELF::EM_CRIS:
    return "Axis Communications 32-bit embedded processor";
  case llvm::ELF::EM_JAVELIN:
    return "Infineon Technologies 32-bit embedded processor";
  case llvm::ELF::EM_FIREPATH:
    return "Element 14 64-bit DSP Processor";
  case llvm::ELF::EM_ZSP:
    return "LSI Logic 16-bit DSP Processor";
  case llvm::ELF::EM_MMIX:
    return "Donald Knuth's educational 64-bit processor";
  case llvm::ELF::EM_HUANY:
    return "Harvard University machine-independent object files";
  case llvm::ELF::EM_PRISM:
    return "SiTera Prism";
  case llvm::ELF::EM_AVR:
    return "Atmel AVR 8-bit microcontroller";
  case llvm::ELF::EM_FR30:
    return "Fujitsu FR30";
  case llvm::ELF::EM_D10V:
    return "Mitsubishi D10V";
  case llvm::ELF::EM_D30V:
    return "Mitsubishi D30V";
  case llvm::ELF::EM_V850:
    return "NEC v850";
  case llvm::ELF::EM_M32R:
    return "Mitsubishi M32R";
  case llvm::ELF::EM_MN10300:
    return "Matsushita MN10300";
  case llvm::ELF::EM_MN10200:
    return "Matsushita MN10200";
  case llvm::ELF::EM_PJ:
    return "picoJava";
  case llvm::ELF::EM_OPENRISC:
    return "OpenRISC 32-bit embedded processor";
  case llvm::ELF::EM_ARC_COMPACT:
    return "ARC International ARCompact processor (old spelling/synonym: "
           "EM_ARC_A5)";
  case llvm::ELF::EM_XTENSA:
    return "Tensilica Xtensa Architecture";
  case llvm::ELF::EM_VIDEOCORE:
    return "Alphamosaic VideoCore processor";
  case llvm::ELF::EM_TMM_GPP:
    return "Thompson Multimedia General Purpose Processor";
  case llvm::ELF::EM_NS32K:
    return "National Semiconductor 32000 series";
  case llvm::ELF::EM_TPC:
    return "Tenor Network TPC processor";
  case llvm::ELF::EM_SNP1K:
    return "Trebia SNP 1000 processor";
  case llvm::ELF::EM_ST200:
    return "STMicroelectronics (www.st.com) ST200";
  case llvm::ELF::EM_IP2K:
    return "Ubicom IP2xxx microcontroller family";
  case llvm::ELF::EM_MAX:
    return "MAX Processor";
  case llvm::ELF::EM_CR:
    return "National Semiconductor CompactRISC microprocessor";
  case llvm::ELF::EM_F2MC16:
    return "Fujitsu F2MC16";
  case llvm::ELF::EM_MSP430:
    return "Texas Instruments embedded microcontroller msp430";
  case llvm::ELF::EM_BLACKFIN:
    return "Analog Devices Blackfin (DSP) processor";
  case llvm::ELF::EM_SE_C33:
    return "S1C33 Family of Seiko Epson processors";
  case llvm::ELF::EM_SEP:
    return "Sharp embedded microprocessor";
  case llvm::ELF::EM_ARCA:
    return "Arca RISC Microprocessor";
  case llvm::ELF::EM_UNICORE:
    return "Microprocessor series from PKU-Unity Ltd. and MPRC of Peking "
           "University";
  case llvm::ELF::EM_EXCESS:
    return "eXcess: 16/32/64-bit configurable embedded CPU";
  case llvm::ELF::EM_DXP:
    return "Icera Semiconductor Inc. Deep Execution Processor";
  case llvm::ELF::EM_ALTERA_NIOS2:
    return "Altera Nios II soft-core processor";
  case llvm::ELF::EM_CRX:
    return "National Semiconductor CompactRISC CRX";
  case llvm::ELF::EM_XGATE:
    return "Motorola XGATE embedded processor";
  case llvm::ELF::EM_C166:
    return "Infineon C16x/XC16x processor";
  case llvm::ELF::EM_M16C:
    return "Renesas M16C series microprocessors";
  case llvm::ELF::EM_DSPIC30F:
    return "Microchip Technology dsPIC30F Digital Signal";
  case llvm::ELF::EM_CE:
    return "Freescale Communication Engine RISC core";
  case llvm::ELF::EM_M32C:
    return "Renesas M32C series microprocessors";
  case llvm::ELF::EM_TSK3000:
    return "Altium TSK3000 core";
  case llvm::ELF::EM_RS08:
    return "Freescale RS08 embedded processor";
  case llvm::ELF::EM_SHARC:
    return "Analog Devices SHARC family of 32-bit DSP processors";
  case llvm::ELF::EM_ECOG2:
    return "Cyan Technology eCOG2 microprocessor";
  case llvm::ELF::EM_SCORE7:
    return "Sunplus S+core7 RISC processor";
  case llvm::ELF::EM_DSP24:
    return "New Japan Radio (NJR) 24-bit DSP Processor";
  case llvm::ELF::EM_VIDEOCORE3:
    return "Broadcom VideoCore III processor";
  case llvm::ELF::EM_LATTICEMICO32:
    return "RISC processor for Lattice FPGA architecture";
  case llvm::ELF::EM_SE_C17:
    return "Seiko Epson C17 family";
  case llvm::ELF::EM_TI_C6000:
    return "The Texas Instruments TMS320C6000 DSP family";
  case llvm::ELF::EM_TI_C2000:
    return "The Texas Instruments TMS320C2000 DSP family";
  case llvm::ELF::EM_TI_C5500:
    return "The Texas Instruments TMS320C55x DSP family";
  case llvm::ELF::EM_MMDSP_PLUS:
    return "STMicroelectronics 64bit VLIW Data Signal Processor";
  case llvm::ELF::EM_CYPRESS_M8C:
    return "Cypress M8C microprocessor";
  case llvm::ELF::EM_R32C:
    return "Renesas R32C series microprocessors";
  case llvm::ELF::EM_TRIMEDIA:
    return "NXP Semiconductors TriMedia architecture family";
  case llvm::ELF::EM_HEXAGON:
    return "Qualcomm Hexagon processor";
  case llvm::ELF::EM_8051:
    return "Intel 8051 and variants";
  case llvm::ELF::EM_STXP7X:
    return "STMicroelectronics STxP7x family of configurable and extensible "
           "RISC processors";
  case llvm::ELF::EM_NDS32:
    return "Andes Technology compact code size embedded RISC processor family";
  case llvm::ELF::EM_ECOG1:
    return "Cyan Technology eCOG1X family";
  case llvm::ELF::EM_MAXQ30:
    return "Dallas Semiconductor MAXQ30 Core Micro-controllers";
  case llvm::ELF::EM_XIMO16:
    return "New Japan Radio (NJR) 16-bit DSP Processor";
  case llvm::ELF::EM_MANIK:
    return "M2000 Reconfigurable RISC Microprocessor";
  case llvm::ELF::EM_CRAYNV2:
    return "Cray Inc. NV2 vector architecture";
  case llvm::ELF::EM_RX:
    return "Renesas RX family";
  case llvm::ELF::EM_METAG:
    return "Imagination Technologies META processor architecture";
  case llvm::ELF::EM_MCST_ELBRUS:
    return "MCST Elbrus general purpose hardware architecture";
  case llvm::ELF::EM_ECOG16:
    return "Cyan Technology eCOG16 family";
  case llvm::ELF::EM_CR16:
    return "National Semiconductor CompactRISC CR16 16-bit microprocessor";
  case llvm::ELF::EM_ETPU:
    return "Freescale Extended Time Processing Unit";
  case llvm::ELF::EM_SLE9X:
    return "Infineon Technologies SLE9X core";
  case llvm::ELF::EM_L10M:
    return "Intel L10M";
  case llvm::ELF::EM_K10M:
    return "Intel K10M";
  case llvm::ELF::EM_AARCH64:
    return "ARM AArch64";
  case llvm::ELF::EM_AVR32:
    return "Atmel Corporation 32-bit microprocessor family";
  case llvm::ELF::EM_STM8:
    return "STMicroeletronics STM8 8-bit microcontroller";
  case llvm::ELF::EM_TILE64:
    return "Tilera TILE64 multicore architecture family";
  case llvm::ELF::EM_TILEPRO:
    return "Tilera TILEPro multicore architecture family";
  case llvm::ELF::EM_MICROBLAZE:
    return "Xilinx MicroBlaze 32-bit RISC soft processor core";
  case llvm::ELF::EM_CUDA:
    return "NVIDIA CUDA architecture";
  case llvm::ELF::EM_TILEGX:
    return "Tilera TILE-Gx multicore architecture family";
  case llvm::ELF::EM_CLOUDSHIELD:
    return "CloudShield architecture family";
  case llvm::ELF::EM_COREA_1ST:
    return "KIPO-KAIST Core-A 1st generation processor family";
  case llvm::ELF::EM_COREA_2ND:
    return "KIPO-KAIST Core-A 2nd generation processor family";
  case llvm::ELF::EM_ARC_COMPACT2:
    return "Synopsys ARCompact V2";
  case llvm::ELF::EM_OPEN8:
    return "Open8 8-bit RISC soft processor core";
  case llvm::ELF::EM_RL78:
    return "Renesas RL78 family";
  case llvm::ELF::EM_VIDEOCORE5:
    return "Broadcom VideoCore V processor";
  case llvm::ELF::EM_78KOR:
    return "Renesas 78KOR family";
  case llvm::ELF::EM_56800EX:
    return "Freescale 56800EX Digital Signal Controller (DSC)";
  case llvm::ELF::EM_BA1:
    return "Beyond BA1 CPU architecture";
  case llvm::ELF::EM_BA2:
    return "Beyond BA2 CPU architecture";
  case llvm::ELF::EM_XCORE:
    return "XMOS xCORE processor family";
  case llvm::ELF::EM_MCHP_PIC:
    return "Microchip 8-bit PIC(r) family";
  case llvm::ELF::EM_INTEL205:
    return "Reserved by Intel";
  case llvm::ELF::EM_INTEL206:
    return "Reserved by Intel";
  case llvm::ELF::EM_INTEL207:
    return "Reserved by Intel";
  case llvm::ELF::EM_INTEL208:
    return "Reserved by Intel";
  case llvm::ELF::EM_INTEL209:
    return "Reserved by Intel";
  case llvm::ELF::EM_KM32:
    return "KM211 KM32 32-bit processor";
  case llvm::ELF::EM_KMX32:
    return "KM211 KMX32 32-bit processor";
  case llvm::ELF::EM_KMX16:
    return "KM211 KMX16 16-bit processor";
  case llvm::ELF::EM_KMX8:
    return "KM211 KMX8 8-bit processor";
  case llvm::ELF::EM_KVARC:
    return "KM211 KVARC processor";
  case llvm::ELF::EM_CDP:
    return "Paneve CDP architecture family";
  case llvm::ELF::EM_COGE:
    return "Cognitive Smart Memory Processor";
  case llvm::ELF::EM_COOL:
    return "iCelero CoolEngine";
  case llvm::ELF::EM_NORC:
    return "Nanoradio Optimized RISC";
  case llvm::ELF::EM_CSR_KALIMBA:
    return "CSR Kalimba architecture family";
  case llvm::ELF::EM_AMDGPU:
    return "AMD GPU architecture";
  case llvm::ELF::EM_RISCV:
    return "RISC-V";
  case llvm::ELF::EM_LANAI:
    return "Lanai 32-bit processor";
  case llvm::ELF::EM_BPF:
    return "Linux kernel bpf virtual machine";
  case llvm::ELF::EM_VE:
    return "NEC SX-Aurora VE";
  case llvm::ELF::EM_CSKY:
    return "C-SKY 32-bit processor";
  case llvm::ELF::EM_LOONGARCH:
    return "LoongArch";
  default:
    return "UNKNOWN";
  }
}

std::string TypeToString::sectionFlagToString(const uint64_t flag) {
  std::string res;
  if (flag & llvm::ELF::SHF_WRITE) {
    res += "W";
  }
  if (flag & llvm::ELF::SHF_ALLOC) {
    res += "A";
  }
  if (flag & llvm::ELF::SHF_EXECINSTR) {
    res += "X";
  }
  if (flag & llvm::ELF::SHF_MERGE) {
    res += "M";
  }
  if (flag & llvm::ELF::SHF_STRINGS) {
    res += "S";
  }
  if (flag & llvm::ELF::SHF_INFO_LINK) {
    res += "I";
  }
  if (flag & llvm::ELF::SHF_LINK_ORDER) {
    res += "L";
  }
  if (flag & llvm::ELF::SHF_OS_NONCONFORMING) {
    res += "O";
  }
  if (flag & llvm::ELF::SHF_GROUP) {
    res += "G";
  }
  if (flag & llvm::ELF::SHF_TLS) {
    res += "T";
  }
  if (flag & llvm::ELF::SHF_COMPRESSED) {
    res += "C";
  }
  if ((flag & llvm::ELF::SHF_GNU_RETAIN) || (flag & llvm::ELF::SHF_EXCLUDE) ||
      (flag & llvm::ELF::SHF_MASKOS) ||
      (flag & llvm::ELF::SHF_SUNW_NODISCARD) ||
      (flag & llvm::ELF::SHF_MASKPROC) ||
      (flag & llvm::ELF::XCORE_SHF_DP_SECTION) ||
      (flag & llvm::ELF::XCORE_SHF_CP_SECTION) ||
      (flag & llvm::ELF::SHF_X86_64_LARGE) ||
      (flag & llvm::ELF::SHF_HEX_GPREL) ||
      (flag & llvm::ELF::SHF_MIPS_NODUPES) ||
      (flag & llvm::ELF::SHF_MIPS_NAMES) ||
      (flag & llvm::ELF::SHF_MIPS_LOCAL) ||
      (flag & llvm::ELF::SHF_MIPS_NOSTRIP) ||
      (flag & llvm::ELF::SHF_MIPS_GPREL) ||
      (flag & llvm::ELF::SHF_MIPS_MERGE) || (flag & llvm::ELF::SHF_MIPS_ADDR) ||
      (flag & llvm::ELF::SHF_MIPS_STRING) ||
      (flag & llvm::ELF::SHF_ARM_PURECODE)) {
    res += "?";
  }
  return res;
}

std::string TypeToString::sectionTypeToString(const uint32_t type) {
  switch (type) {
  case llvm::ELF::SHT_NULL:
    return "NULL";
  case llvm::ELF::SHT_PROGBITS:
    return "PROGBITS";
  case llvm::ELF::SHT_SYMTAB:
    return "SYMTAB";
  case llvm::ELF::SHT_STRTAB:
    return "STRTAB";
  case llvm::ELF::SHT_RELA:
    return "RELA";
  case llvm::ELF::SHT_HASH:
    return "HASH";
  case llvm::ELF::SHT_DYNAMIC:
    return "DYNSYM";
  case llvm::ELF::SHT_NOTE:
    return "NOTE";
  case llvm::ELF::SHT_NOBITS:
    return "NOBITS";
  case llvm::ELF::SHT_REL:
    return "REL";
  case llvm::ELF::SHT_SHLIB:
    return "SHLIB";
  case llvm::ELF::SHT_DYNSYM:
    return "DYNSYM";
  case llvm::ELF::SHT_INIT_ARRAY:
    return "INIT_ARRAY";
  case llvm::ELF::SHT_FINI_ARRAY:
    return "FINI_ARRAY";
  case llvm::ELF::SHT_PREINIT_ARRAY:
    return "PREINIT_ARRAY";
  case llvm::ELF::SHT_GROUP:
    return "GROUP";
  case llvm::ELF::SHT_SYMTAB_SHNDX:
    return "SYMTAB_SHNDX";
  case llvm::ELF::SHT_RELR:
    return "RELR";
  case llvm::ELF::SHT_LOOS:
    return "LOOS";
  case llvm::ELF::SHT_ANDROID_REL:
    return "ANDROID_REL";
  case llvm::ELF::SHT_ANDROID_RELA:
    return "ANDROID_RELA";
  case llvm::ELF::SHT_LLVM_ODRTAB:
    return "LLVM_ODRTAB";
  case llvm::ELF::SHT_LLVM_LINKER_OPTIONS:
    return "LLVM_LINKER_OPTIONS";
  case llvm::ELF::SHT_LLVM_ADDRSIG:
    return "LLVM_ADDRSIG";
  case llvm::ELF::SHT_LLVM_DEPENDENT_LIBRARIES:
    return "LLVM_DEPENDENT_LIBRARIES";
  case llvm::ELF::SHT_LLVM_SYMPART:
    return "LLVM_SYMPART";
  case llvm::ELF::SHT_LLVM_PART_EHDR:
    return "LLVM_PART_EHDR";
  case llvm::ELF::SHT_LLVM_PART_PHDR:
    return "LLVM_PART_PHDR";
  case llvm::ELF::SHT_LLVM_BB_ADDR_MAP_V0:
    return "LLVM_BB_ADDR_MAP_V0";
  case llvm::ELF::SHT_LLVM_CALL_GRAPH_PROFILE:
    return "LLVM_CALL_GRAPH_PROFILE";
  case llvm::ELF::SHT_LLVM_BB_ADDR_MAP:
    return "LLVM_BB_ADDR_MAP";
  case llvm::ELF::SHT_LLVM_OFFLOADING:
    return "LLVM_OFFLOADING";
  case llvm::ELF::SHT_ANDROID_RELR:
    return "ANDROID_RELR";
  case llvm::ELF::SHT_GNU_ATTRIBUTES:
    return "GNU_ATTRIBUTES";
  case llvm::ELF::SHT_GNU_HASH:
    return "GNU_HASH";
  case llvm::ELF::SHT_GNU_verdef:
    return "GNU_VERDEF";
  case llvm::ELF::SHT_GNU_verneed:
    return "GNU_VERNEED";
  case llvm::ELF::SHT_GNU_versym:
    return "GNU_VERSYM/HIOS";
  // case llvm::ELF::SHT_HIOS:
  //   return "HIOS";
  case llvm::ELF::SHT_LOPROC:
    return "LOPROC/HEX_ORDERED";
  case llvm::ELF::SHT_ARM_EXIDX:
    // return "ARM_EXIDX/X86_64_UNWIND/CSKY_ATTRIBUTES";
    return "X86_64_UNWIND";
  case llvm::ELF::SHT_ARM_PREEMPTMAP:
    return "ARM_PREEMPTMAP";
  case llvm::ELF::SHT_ARM_ATTRIBUTES:
    // return "ARM_ATTRIBUTES/MSP430_ATTRIBUTES/RISCV_ATTRIBUTES";
    return "ARM_ATTRIBUTES";
  case llvm::ELF::SHT_ARM_DEBUGOVERLAY:
    return "ARM_DEBUG_OVERLAY";
  case llvm::ELF::SHT_ARM_OVERLAYSECTION:
    return "ARM_OVERLAYSECTION";
  // case llvm::ELF::SHT_AARCH64_MEMTAG_GLOBALS_STATIC:
  //   return "AARCH64_MEMTAG_GLOBALS_STATIC";
  // case llvm::ELF::SHT_AARCH64_MEMTAG_GLOBALS_DYNAMIC:
  //   return "AARCH64_MEMTAG_GLOBALS_DYNAMIC";
  // case llvm::ELF::SHT_HEX_ORDERED:
  //   return "HEX_ORDERED";
  // case llvm::ELF::SHT_X86_64_UNWIND:
  //   return "X86_64_UNWIND";
  case llvm::ELF::SHT_MIPS_REGINFO:
    return "MIPS_REGINFO";
  case llvm::ELF::SHT_MIPS_OPTIONS:
    return "MIPS_OPTIONS";
  case llvm::ELF::SHT_MIPS_DWARF:
    return "MIPS_DWARF";
  case llvm::ELF::SHT_MIPS_ABIFLAGS:
    return "MIPS_ABIFLAGS";
  // case llvm::ELF::SHT_MSP430_ATTRIBUTES:
  //   return "MSP430_ATTRIBUTES/RISCV_ATTRIBUTES";
  // case llvm::ELF::SHT_RISCV_ATTRIBUTES:
  //   return "RISCV_ATTRIBUTES";
  // case llvm::ELF::SHT_CSKY_ATTRIBUTES:
  //   return "CSKY_ATTRIBUTES";
  case llvm::ELF::SHT_HIPROC:
    return "HIPROC";
  case llvm::ELF::SHT_LOUSER:
    return "LOUSER";
  case llvm::ELF::SHT_HIUSER:
    return "HIUSER";
  default:
    return "UNKNOWN";
  }
}

std::string TypeToString::symbolTypeToString(const unsigned char type) {
  switch (type) {
  case llvm::ELF::STT_NOTYPE:
    return "NOTYPE";
  case llvm::ELF::STT_OBJECT:
    return "OBJECT";
  case llvm::ELF::STT_FUNC:
    return "FUNC";
  case llvm::ELF::STT_SECTION:
    return "SECTION";
  case llvm::ELF::STT_FILE:
    return "FILE";
  case llvm::ELF::STT_COMMON:
    return "COMMON";
  case llvm::ELF::STT_TLS:
    return "TLS";
  case llvm::ELF::STT_GNU_IFUNC:
    // return "GNU_IFUNC/LOOS/AMDGPU_HSA_KERNEL";
    return "LOOS";
  case llvm::ELF::STT_HIOS:
    return "HIOS";
  case llvm::ELF::STT_LOPROC:
    return "LOPROC";
  case llvm::ELF::STT_HIPROC:
    return "HIPROC";
  default:;
    return "UNKNOWN";
  }
}

std::string TypeToString::symbolBindToString(const unsigned char type) {
  switch (type) {
  case llvm::ELF::STB_LOCAL:
    return "LOCAL";
  case llvm::ELF::STB_GLOBAL:
    return "GLOBAL";
  case llvm::ELF::STB_WEAK:
    return "WEAK";
  case llvm::ELF::STB_GNU_UNIQUE:
    // return "GNU_UNIQUE/LOOS";
    return "LOOS";
  case llvm::ELF::STB_HIOS:
    return "HIOS";
  case llvm::ELF::STB_LOPROC:
    return "LOPROC";
  case llvm::ELF::STB_HIPROC:
    return "HIPROC";
  default:
    return "UNKNOWN";
  }
}

std::string TypeToString::symbolVisToString(const unsigned char type) {
  switch (type) {
  case llvm::ELF::STV_DEFAULT:
    return "DEFAULT";
  case llvm::ELF::STV_INTERNAL:
    return "INTERNAL";
  case llvm::ELF::STV_HIDDEN:
    return "HIDDEN";
  case llvm::ELF::STV_PROTECTED:
    return "PROTECTED";
  default:
    return "UNKNOWN";
  }
}

std::string TypeToString::symbolShndxToString(const int type) {
  switch (type) {
  case llvm::ELF::SHN_UNDEF:
    return "UND";
  case llvm::ELF::SHN_LORESERVE:
    // return "LOS/LOP";
    return "LOS";
  case llvm::ELF::SHN_HIPROC:
    return "HIPROC";
  case llvm::ELF::SHN_LOOS:
    return "LOOS";
  case llvm::ELF::SHN_HIOS:
    return "HIOS";
  case llvm::ELF::SHN_ABS:
    return "ABS";
  case llvm::ELF::SHN_COMMON:
    return "COMMON";
  default:
    return "UNKNOWN";
  }
}

std::string TypeToString::relaTypeToString(const uint32_t type) {
  switch (type) {
  case llvm::ELF::R_X86_64_NONE:
    return "R_X86_64_NONE";
  case llvm::ELF::R_X86_64_64:
    return "R_X86_64_64";
  case llvm::ELF::R_X86_64_PC32:
    return "R_X86_64_PC32";
  case llvm::ELF::R_X86_64_GOT32:
    return "R_X86_64_GOT32";
  case llvm::ELF::R_X86_64_PLT32:
    return "R_X86_64_PLT32";
  case llvm::ELF::R_X86_64_COPY:
    return "R_X86_64_COPY";
  case llvm::ELF::R_X86_64_GLOB_DAT:
    return "R_X86_64_GLOB_DAT";
  case llvm::ELF::R_X86_64_JUMP_SLOT:
    return "R_X86_64_JUMP_SLOT";
  case llvm::ELF::R_X86_64_RELATIVE:
    return "R_X86_64_RELATIVE";
  case llvm::ELF::R_X86_64_GOTPCREL:
    return "R_X86_64_GOTPCREL";
  case llvm::ELF::R_X86_64_32:
    return "R_X86_64_32";
  case llvm::ELF::R_X86_64_32S:
    return "R_X86_64_32S";
  case llvm::ELF::R_X86_64_16:
    return "R_X86_64_16";
  case llvm::ELF::R_X86_64_PC16:
    return "R_X86_64_PC16";
  case llvm::ELF::R_X86_64_8:
    return "R_X86_64_8";
  case llvm::ELF::R_X86_64_PC8:
    return "R_X86_64_PC8";
  case llvm::ELF::R_X86_64_DTPMOD64:
    return "R_X86_64_DTPMOD64";
  case llvm::ELF::R_X86_64_DTPOFF64:
    return "R_X86_64_DTPOFF64";
  case llvm::ELF::R_X86_64_TPOFF64:
    return "R_X86_64_TPOFF64";
  case llvm::ELF::R_X86_64_TLSGD:
    return "R_X86_64_TLSGD";
  case llvm::ELF::R_X86_64_TLSLD:
    return "R_X86_64_TLSLD";
  case llvm::ELF::R_X86_64_DTPOFF32:
    return "R_X86_64_DTPOFF32";
  case llvm::ELF::R_X86_64_GOTTPOFF:
    return "R_X86_64_GOTTPOFF";
  case llvm::ELF::R_X86_64_TPOFF32:
    return "R_X86_64_TPOFF32";
  case llvm::ELF::R_X86_64_PC64:
    return "R_X86_64_PC64";
  case llvm::ELF::R_X86_64_GOTOFF64:
    return "R_X86_64_GOTOFF64";
  case llvm::ELF::R_X86_64_GOTPC32:
    return "R_X86_64_GOTPC32";
  case llvm::ELF::R_X86_64_GOT64:
    return "R_X86_64_GOT64";
  case llvm::ELF::R_X86_64_GOTPCREL64:
    return "R_X86_64_GOTPCREL64";
  case llvm::ELF::R_X86_64_GOTPC64:
    return "R_X86_64_GOTPC64";
  case llvm::ELF::R_X86_64_GOTPLT64:
    return "R_X86_64_GOTPLT64";
  case llvm::ELF::R_X86_64_PLTOFF64:
    return "R_X86_64_PLTOFF64";
  case llvm::ELF::R_X86_64_SIZE32:
    return "R_X86_64_SIZE32";
  case llvm::ELF::R_X86_64_SIZE64:
    return "R_X86_64_SIZE64";
  case llvm::ELF::R_X86_64_GOTPC32_TLSDESC:
    return "R_X86_64_GOTPC32_TLSDESC";
  case llvm::ELF::R_X86_64_TLSDESC_CALL:
    return "R_X86_64_TLSDESC_CALL";
  case llvm::ELF::R_X86_64_TLSDESC:
    return "R_X86_64_TLSDESC";
  case llvm::ELF::R_X86_64_IRELATIVE:
    return "R_X86_64_IRELATIVE";
  case llvm::ELF::R_X86_64_GOTPCRELX:
    return "R_X86_64_GOTPCRELX";
  case llvm::ELF::R_X86_64_REX_GOTPCRELX:
    return "R_X86_64_REX_GOTPCRELX";
  default:
    return "UNKNOWN";
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm
