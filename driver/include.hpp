#pragma once

// Windows
#include <ntifs.h>
#include <ntddk.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <ntstrsafe.h>
#include <ntimage.h>

// C++
#include <intrin.h>
#include <xtr1common>

// Project
#include <defs.hpp>
#include <shared.hpp>

#include <kernel/nt.hpp>
#include <kernel/crt.hpp>
#include <kernel/scanner.hpp>
#include <kernel/modules.hpp>

#include <hide/offsets.hpp>
#include <hide/ret.hpp>
#include <hide/thread.hpp>

#include <kernel/physical/physical.hpp>
#include <kernel/physical/cr3.hpp>

#include <comm/comm.hpp>