
#pragma once

#include "Algorithm.hh"

class rewrite_indices : public Algorithm {
	public:
		rewrite_indices(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		Ex          preferred;
		bool        is_derivative_argument;
};
