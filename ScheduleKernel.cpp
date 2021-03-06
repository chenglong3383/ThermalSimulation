#include "ScheduleKernel.h"
#include "vectormath.h"
#include "Statistics.h"

ScheduleKernel::ScheduleKernel(unsigned n, 
	enum _schedule_kernel _kernel_type){
	nstages = n;
	kernel_type = _kernel_type;

	timeExpense.reserve(10000);
	// initialized = false;
}


void ScheduleKernel::setScheduler(Scheduler* s){
	scheduler = s;
}


void ScheduleKernel::getScheme(vector<double> & tons, 
	vector<double>& toffs, pipeinfo& config){
	double timein = (double)Statistics::getRelativeTime_ms();
	ScheduleKernel *t = (ScheduleKernel *) this;
	t->getScheduleScheme( tons,  toffs,  config);
	double timeout = (double)Statistics::getRelativeTime_ms();
	timeExpense.push_back(timeout - timein);
}


double ScheduleKernel::getMaxTimeExpense(){
	return maxElement(timeExpense);
}

double ScheduleKernel::getMeanTimeExpense(){
	double sum = std::accumulate(timeExpense.begin(), timeExpense.end(), 0.0);
	double avg = sum/timeExpense.size();
	return avg;
}

