#include <stdio.h>
#include <math.h>


#include <vector>



using namespace std;


int 
main ( )
{
  vector<double> numbers;

  double min,
         max,
         number,
         sum   = 0,
         count = 0;

  while ( 1 == fscanf ( stdin, "%lf", & number ) )
  {
    if ( ! count )
      min = max = number;
    else
    {
      min = ( number < min ) ? number : min;
      max = ( number > max ) ? number : max;
    }

    count ++;
    sum   += number;
    numbers.push_back ( number );
  }

  double mean = sum / count;
  double diff,
         sum_of_squared_diffs = 0;

  vector<double>::iterator i;
  for ( i = numbers.begin(); i < numbers.end(); ++ i )
  {
    diff = *i - mean;
    sum_of_squared_diffs += diff * diff;
  }


  double sigma = sqrt ( sum_of_squared_diffs / count );

  fprintf ( stdout, 
            "count %lf   sum %lf   mean %lf   sigma %lf   min %lf   max %lf  sigma_over_mean %lf\n",
            count,
            sum,
            mean,
            sigma,
            min,
            max,
            sigma / mean
          );

  return 0;
}





