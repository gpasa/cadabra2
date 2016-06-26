
#include "Weight.hh"

std::string Weight::name() const 
	{
	return "Weight";
	}

bool Weight::parse(const Kernel& k, keyval_t& kv)
	{
	keyval_t::const_iterator kvit=kv.find("value");
	if(kvit!=kv.end()) value_=*kvit->second->multiplier;
	else               value_=1;

	return WeightBase::parse(k, kv);
	}

multiplier_t Weight::value(const Kernel& pr, Ex::iterator, const std::string& forcedlabel) const
	{
	if(forcedlabel!=label) return -1;
	return value_;
	}
