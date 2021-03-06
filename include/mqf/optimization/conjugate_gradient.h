#ifndef INCLUDED_MQF_OPTIMIZATION_CONJUGATE_GRADIENT
#define INCLUDED_MQF_OPTIMIZATION_CONJUGATE_GRADIENT
#include "linesearch.h"

namespace mqf {

	// tags for the CG schemes
	struct FletcherReeves {};
	struct PolakRibiere {};
	struct HestenesStiefel {};
	struct ConjugateDescent {};
	struct DaiYuan {};

	/*
	 * Conjugate Gradient on Riemannian Manifolds
	 *
	 * Seeks a local minimum of a smooth cost function S : M -> R by stepping along
	 * geodesics in the direction determined by the Conjugate Gradient scheme.
	 *
	 */
	template<typename Geodesic,typename Scheme = HestenesStiefel>
	struct ConjugateGradient {
		using Metric = typename Geodesic::Metric;
		using Point = typename Metric::Point;
		using Vector = typename Metric::InnerProduct::Vector;

		Metric metric;
		Geodesic geodesic;
		LineSearch lineSearch;

		uint32_t n = 0,
		         maxSteps = 1000;

		Point x, lastX;
		Vector grad, lastGrad, velocity, ptLastVel;

		template<typename S,typename DS>
		bool step( S&& cost, DS&& gradient ) {
			using std::swap;

			// update the gradient
			swap( lastGrad, grad );
			grad = gradient(x);

			// the CG velocity is the negative gradient modified by the previous velocity
			velocity = -grad;
			if( n > 0 ) {
				ptLastVel = geodesic.parallelTranslate( geodesic.velocity, lineSearch.alpha );
				velocity += ptLastVel * modifier<Scheme>();
			}

			// set the geodesic
			geodesic.set( x, velocity );

			// line search the geodesic
			double alpha = lineSearch.search(
				[&]( double t ) {
					return cost( geodesic(t) );
				},
				[&]( double t ) {
					Point xt = geodesic(t);
					return metric(xt)(
						gradient(xt),
						geodesic.parallelTranslate( geodesic.velocity, t )
					);
				}
			);

			if( alpha <= 0.0 ) return false;

			// step to the new position
			swap( lastX, x );
			x = geodesic(alpha);

			return true;
		}

		template<typename S,typename DS>
		Point optimize( const Point& initial, const S& cost, const DS& gradient ) {
			x = initial;
			lineSearch.reset();
			for(n=0;n<maxSteps;++n) {
				if( !step( cost, gradient ) )
					break;
			}
			return x;
		}

	protected:

		template<typename>
		double modifier();

		template<>
		double modifier<FletcherReeves>() {
			return metric(x).norm2(grad) / metric(lastX).norm2(lastGrad);
		}

		template<>
		double modifier<PolakRibiere>() {
			auto innerProduct = metric(x);
			auto ptlastGrad = geodesic.parallelTranslate( lastGrad, lineSearch.alpha );
			return innerProduct( grad, grad - ptlastGrad ) / innerProduct.norm2( ptlastGrad );
		}

		template<>
		double modifier<HestenesStiefel>() {
			auto innerProduct = metric(x);
			auto ptlastGrad = geodesic.parallelTranslate( lastGrad, lineSearch.alpha );
			auto diff = grad - ptlastGrad;
			return innerProduct( grad, diff ) / innerProduct( ptLastVel, diff );
		}

		template<>
		double modifier<ConjugateDescent>() {
			auto innerProduct = metric(x);
			auto ptlastGrad = geodesic.parallelTranslate( lastGrad, lineSearch.alpha );
			return -innerProduct.norm2( grad ) / innerProduct( ptLastVel, ptlastGrad );
		}
		
		template<>
		double modifier<DaiYuan>() {
			auto innerProduct = metric(x);
			auto ptlastGrad = geodesic.parallelTranslate( lastGrad, lineSearch.alpha );
			return innerProduct.norm2( grad ) / innerProduct( ptLastVel, grad - ptlastGrad );
		}

	};

}

#endif

