#ifndef Vec3_h
#define Vec3_h

#include <boost/numeric/ublas/vector.hpp>

#include <cmath>

/// A 3-vector wrapper for the ublas static-sized vector.

class Vec3 : public
  boost::numeric::ublas::vector< double, boost::numeric::ublas::bounded_array< double, 3 > >
{
public:
  /// The default constructs a 3-vector.
  Vec3() :
    Vec3BaseType(3)
  {
  }

  /// Initialize with a double.
  Vec3( const double& x ) :
    Vec3BaseType(3)
  {
    for ( int i = 0; i < 3; ++i )
      (*this)[i] = x;
  }

private:
  typedef boost::numeric::ublas::vector< double, boost::numeric::ublas::bounded_array< double, 3 > >
    Vec3BaseType;

  // Override and hide these since we don't want the size to change.
  void resize( size_type size, bool preserve = true );
  void insert_element( size_type i, const_reference t );
  void erase_element( size_type i );
  void clear();
};

double magnitudeSquared( const Vec3& v )
{
  return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

double magnitude( const Vec3& v )
{
  return sqrt( magnitudeSquared( v ) );
}

#endif // Vec3_h
