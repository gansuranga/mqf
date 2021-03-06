#ifndef INCLUDED_MQF_TRADING_STRAT_BASIC_MA
#define INCLUDED_MQF_TRADING_STRAT_BASIC_MA
#include "../action.h"
#include "../../time_series/wma.h"

namespace mqf {
namespace Strategies {

	struct BasicMA {
		int shortPeriod, longPeriod;

		explicit BasicMA( int shortPeriod = 50,
		                  int longPeriod = 100 ) :
			shortPeriod(shortPeriod),
			longPeriod(longPeriod)
		{}

		template<typename It>
		Action compute( It p1, It p2 ) const {
			auto count = std::distance(p1,p2);
			if( count < std::max(shortPeriod,longPeriod) )
				return Action( Action::Out );
		
			auto shortMA = WMA(shortPeriod).back(p1,p2);
			auto longMA = WMA(longPeriod).back(p1,p2);

			if( shortMA > longMA ) {
				return Action( Action::Long );
			}
			return Action( Action::Out );
		}

	};

}
}

#endif
