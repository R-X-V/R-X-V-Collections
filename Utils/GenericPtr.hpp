#pragma once


class GenericPointer {
public:
	GenericPointer( ) : value { nullptr } { }

	template <typename T>
	explicit( false ) GenericPointer( T* pointer ) noexcept
		: value { ( void* ) pointer } 
	{ }

	explicit( false ) GenericPointer( uintptr_t pointer ) noexcept
		: value { ( void* ) pointer } { }


	[[nodiscard]] GenericPointer operator +(std::ptrdiff_t off ) const noexcept {
		return { reinterpret_cast< std::uintptr_t >( value ) + off };
	}

	[[nodiscard]] explicit(true) operator bool( ) const noexcept {
		return value != nullptr;
	}

	template <typename T>
	[[nodiscard]] T as( ) const noexcept {

		static_assert( std::is_pointer_v<T> || std::is_integral_v<T>, "T must be a pointer or an integral" );
		return T( value );
	}


	[[nodiscard]] inline std::uintptr_t GetRawAddress( ) const noexcept {
		return reinterpret_cast< std::uintptr_t >( value);
	}
	
private: 
	void* value;
};