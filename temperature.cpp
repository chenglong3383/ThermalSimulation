#include "temperature.h"
#include "Pipeline.h"
#include "Scratch.h"
#include "vectormath.h"
#include "Priorities.h"
#include "Semaphores.h"

#include <iostream>
#include <fstream>
#include <sstream>



using namespace std;


#define _INFO 1

#define _DEBUG 1



TempWatcher::TempWatcher(unsigned period, string _filename, unsigned _id): TimedRunnable(_id){

	unsigned long sim_length = Scratch::getDuration();
	if (period == 0){
		cerr << "TempWatcher::TempWatcher: Error, period is zero!\n";
		exit(-1);
	}
  sem_init(&temp_sem, 0, 1);

  sem_wait(&temp_sem);
  curTemp = get_cpu_temperature();
  sem_post(&temp_sem);
  unsigned count           = sim_length/period;
  for (unsigned i = 0; i < count; ++i){
    reading_times.push_back((unsigned long) i*period);
  }

  filename = _filename;

}	

void TempWatcher::activate(){
	setPriority(Priorities::get_server_pr());
}

void TempWatcher::join(){
	join2();
}


void TempWatcher::wrapper(){

  #if _INFO == 1
  Semaphores::print_sem.wait_sem();
  cout << "TempWatcher: " << id << " waiting for initialization\n";
  Semaphores::print_sem.post_sem();
  #endif


  //Wait until the simulation is initialized
  while( !Pipeline::isInitialized() );

  #if _INFO == 1
  Semaphores::print_sem.wait_sem();
  cout << "TempWatcher: " << id << " begining execution \n";
  Semaphores::print_sem.post_sem();
  #endif

	///wait for the simulation start
  while(!Pipeline::isSimulating()){};


  timedRun();

	//toFile();

	#if _INFO == 1
  Semaphores::print_sem.wait_sem();
  cout << "TempWatcher " << id << " exiting wrapper...\n";
  Semaphores::print_sem.post_sem();
  #endif
}

void TempWatcher::setReadingTimes(unsigned long start_time){
  convertRelativeTimesToAbs(reading_times, start_time);
}


void TempWatcher::timedJob(unsigned index){
	sem_wait(&temp_sem);
  curTemp = get_cpu_temperature();
  sem_post(&temp_sem);
  tempTrace.push_back(curTemp);
}


vector<double> TempWatcher::getCurTemp(){
  vector<double> ret;
  sem_wait(&temp_sem);
  ret = curTemp ;
  sem_post(&temp_sem);

  return ret;
}

void TempWatcher::toFile(){

	ofstream file;

  /************ SAVING _temprature_trace *********/
  file.open((filename + "_temprature_trace.csv").data());
  for(unsigned int c=0; c< tempTrace.size();c++) {
  	stringstream out;
    vector<double> tmp = tempTrace[c];
    for (unsigned i = 0; i < tmp.size(); ++i){
    	if ( i != tmp.size()-1 )
    		out << tmp[i] << "," ;
    	else
    		out << tmp[i];
    }

    file << out.str() << endl; 
  }
  file.close();


  //Change the owner and permissions of generated files
  //system(("chown hsf:hsf " + filePrefix + "_*.csv").data() );
  if (system(("chmod 666 " + filename + "_*.csv").data() )){
    cout << "TempWatcher::toFile:error saving file " << endl;
  }

}


std::vector<double> TempWatcher::get_cpu_temperature(){
	#if _DEBUG == 1
    std::vector<double> ret(4, 30);
  #endif

    #if _DEBUG == 0
    std::vector<double> ret;
    int value;
    int TEMP_IDX_MAX = 4;
    FILE *f;
    const char* n[] = {	"/sys/class/hwmon/hwmon1/temp2_input",
    "/sys/class/hwmon/hwmon1/temp3_input",
    "/sys/class/hwmon/hwmon1/temp4_input",
    "/sys/class/hwmon/hwmon1/temp5_input"};

    for ( int i = 0; i < TEMP_IDX_MAX; ++i) {
      if ( ( f = fopen( n[i], "r"))) {
        int res, err = 0;

        errno = 0;
        res = fscanf( f, "%d", &value);
        if ( res == EOF && errno == EIO)
          err = -SENSORS_ERR_IO;
        else if ( res != 1)
          err = -SENSORS_ERR_ACCESS_R;
        res = fclose( f);
        if ( err){
         std::cerr << err << " error"<<std::endl;
         exit(err);
       }

       if ( res == EOF) {
        if ( errno == EIO){
         std::cerr << -SENSORS_ERR_IO << " error"<<std::endl;
         exit(-SENSORS_ERR_IO) ;
       }
       else{
         std::cerr << -SENSORS_ERR_ACCESS_R << " error"<<std::endl;
         exit(-SENSORS_ERR_ACCESS_R) ;
       }
     }
            // value /= get_type_scaling( SENSORS_SUBFEATURE_TEMP_INPUT);
   } else{
     std::cout << -SENSORS_ERR_KERNEL << " error"<<std::endl;
     exit(-SENSORS_ERR_KERNEL) ;
   }

   ret.push_back((double)value);
 }
#endif

return ret;
}

double TempWatcher::getMaxTemp(){
	vector<double> s_max;
	for (unsigned i = 0; i < tempTrace.size(); ++i)
	{
		s_max.push_back(maxElement(tempTrace[i]));
	}
 //return maxElement(s_max);
  double sum = std::accumulate(s_max.begin(), s_max.end(), 0.0);
  double avg = sum/s_max.size();
	
  return avg;

}




// int get_type_scaling(sensors_subfeature_type type)
// {
// 	/* Multipliers for subfeatures */
// 	switch (type & 0xFF80) {
// 	case SENSORS_SUBFEATURE_IN_INPUT:
// 	case SENSORS_SUBFEATURE_TEMP_INPUT:
// 	case SENSORS_SUBFEATURE_CURR_INPUT:
// 	case SENSORS_SUBFEATURE_HUMIDITY_INPUT:
// 		return 1000;
// 	case SENSORS_SUBFEATURE_FAN_INPUT:
// 		return 1;
// 	case SENSORS_SUBFEATURE_POWER_AVERAGE:
// 	case SENSORS_SUBFEATURE_ENERGY_INPUT:
// 		return 1000000;
// 	}

// 	/* Multipliers for second class subfeatures
// 	   that need their own multiplier */
// 	switch (type) {
// 	case SENSORS_SUBFEATURE_POWER_AVERAGE_INTERVAL:
// 	case SENSORS_SUBFEATURE_VID:
// 	case SENSORS_SUBFEATURE_TEMP_OFFSET:
// 		return 1000;
// 	default:
// 		return 1;
// 	}
// }
