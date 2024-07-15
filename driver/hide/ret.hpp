#pragma once
#include <include.hpp>

namespace return_spoofer::detail
{
	extern "C" void* ret_stub();

	template <typename Ret, typename... Args>
	static inline Ret shellcode_stub_helper(const void* shell, Args... args)
	{
		auto fn = (Ret(*)(Args...))(shell);
		return fn(args...);
	}

	template <unsigned long long Argc, typename>
	struct argument_remapper
	{
		// At least 5 params
		template<
			typename Ret,
			typename First,
			typename Second,
			typename Third,
			typename Fourth,
			typename... Pack
		>
		static Ret do_call(
			const void* shell,
			void* shell_param,
			First first,
			Second second,
			Third third,
			Fourth fourth,
			Pack... pack
		)
		{
			return shellcode_stub_helper<
				Ret,
				First,
				Second,
				Third,
				Fourth,
				void*,
				void*,
				Pack...
			>(
				shell,
				first,
				second,
				third,
				fourth,
				shell_param,
				nullptr,
				pack...
			);
		}
	};

	template <unsigned long long Argc>
	struct argument_remapper<Argc, std::enable_if_t<Argc <= 4>>
	{
		// 4 or less params
		template<
			typename Ret,
			typename First = void*,
			typename Second = void*,
			typename Third = void*,
			typename Fourth = void*
		>
		static Ret do_call(
			const void* shell,
			void* shell_param,
			First first = First{},
			Second second = Second{},
			Third third = Third{},
			Fourth fourth = Fourth{}
		)
		{
			return shellcode_stub_helper<
				Ret,
				First,
				Second,
				Third,
				Fourth,
				void*,
				void*
			>(
				shell,
				first,
				second,
				third,
				fourth,
				shell_param,
				nullptr
			);
		}
	};
}

namespace return_spoofer
{
	inline unsigned char* target_base = nullptr;
	
	template <typename result, typename... arguments>
	static inline result call(
		result(*fn)(arguments...),
		arguments... args
	)
	{
		struct shell_params {
			const void* trampoline;
			void* function;
			void* register_; // originally rbx, currently rdx
		};

		shell_params p = { target_base, reinterpret_cast<void*>(fn) };
		using mapper = detail::argument_remapper<sizeof...(arguments), void>;
		return mapper::template do_call<result, arguments...>((const void*)&detail::ret_stub, &p, args...);
	}
}