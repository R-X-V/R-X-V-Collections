#pragma once
#include <vector>

#include "Memory/Patch.hpp"
#include "Memory/Hook.hpp"



class BypassGH {
public:
	static BypassGH& GetBypass( ) noexcept {
		static BypassGH instance;
		return instance;
	}

	void addPatch( std::reference_wrapper<IPatch> patch ) {
		patches.push_back( &patch.get( ) );
	}

	void addHook( std::reference_wrapper<Hook> hook ) {
		hooks.push_back( &hook.get( ) );
	}

	void applyAll( ) {

		for ( auto patch : patches ) {
			patch->apply( );
		}

		for ( auto hook : hooks ) {

			if ( auto installed = hook->install( ); installed )
				hook->enable( );
			else
				assert( installed && "Could not install the hook" );
				
		}
	}

	void restoreAll( ) {

		for ( auto patch : patches ) {
			patch->restore( );
		}

		for ( auto hook : hooks ) {
			hook->disable( );
		}
	}

private:
	BypassGH( ) = default;
	~BypassGH( ) = default;
private:
	std::vector<IPatch*> patches { };
	std::vector<Hook*> hooks { };
};

