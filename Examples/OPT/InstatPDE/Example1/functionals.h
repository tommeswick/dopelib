#ifndef _LOCALFunctionalS_
#define _LOCALFunctionalS_

#include "pdeinterface.h"

#define PI 3.14159265359

using namespace std;
using namespace dealii;
using namespace DOpE;


/****************************************************************************************/

template<typename VECTOR, int dopedim, int dealdim>
  class LocalPointFunctional : public FunctionalInterface<CellDataContainer,FaceDataContainer,dealii::DoFHandler<dealdim>, VECTOR, dopedim,dealdim>
  {
  private:
    mutable double time;

  public:

    void SetTime(double t) const
    {
      time = t;
    }

    bool NeedTime() const
    {
      if (time==0.)
    		return true;
    	else
    		return false;
    }

  double PointValue(const DOpEWrapper::DoFHandler<dopedim, dealii::DoFHandler<dealdim> > &/* control_dof_handler*/,
		    const DOpEWrapper::DoFHandler<dealdim, dealii::DoFHandler<dealdim> > & state_dof_handler,
		    const std::map<std::string, const dealii::Vector<double>* > &/*param_values*/,
		    const std::map<std::string, const VECTOR* > &domain_values)
  {

    Point<2> evaluation_point(0.5*PI,0.5*PI);

    typename map<string, const VECTOR* >::const_iterator it = domain_values.find("state");

    double point_value = VectorTools::point_value(state_dof_handler, *(it->second), evaluation_point);

    // pressure analysis
    return point_value;
  }

  string GetType() const
  {
    return "point timelocal";
    // 1) point domain boundary face
    // 2) timelocal timedistributed
  }
  string GetName() const
  {
    return "Start-Time-Point evaluation";
  }

  };

/************************************************************************************************************************************************/

template<typename VECTOR, int dopedim, int dealdim>
  class LocalPointFunctional2 : public FunctionalInterface<CellDataContainer,FaceDataContainer,dealii::DoFHandler<dealdim>, VECTOR, dopedim,dealdim>
  {
  private:
    mutable double time;

  public:

    void SetTime(double t) const
    {
      time = t;
    }

    bool NeedTime() const
    {
      if (time==1.)
    		return true;
    	else
    		return false;
    }

  double PointValue(const DOpEWrapper::DoFHandler<dopedim, dealii::DoFHandler<dealdim> > &/* control_dof_handler*/,
		    const DOpEWrapper::DoFHandler<dealdim, dealii::DoFHandler<dealdim> > & state_dof_handler,
		    const std::map<std::string, const dealii::Vector<double>* > &/*param_values*/,
		    const std::map<std::string, const VECTOR* > &domain_values)
  {

    Point<2> evaluation_point(0.5*PI,0.5*PI);

    typename map<string, const VECTOR* >::const_iterator it = domain_values.find("state");

    double point_value = VectorTools::point_value(state_dof_handler, *(it->second), evaluation_point);

    // pressure analysis
    return point_value;
  }

  string GetType() const
  {
    return "point timelocal";
    // 1) point domain boundary face
    // 2) timelocal timedistributed
  }
  string GetName() const
  {
    return "End-Time-Point evaluation";
  }

  };


/****************************************************************************************/

#endif