#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <cassert>
#include <cmath>

#ifdef VECOP
#error VECOP already defined!
#endif
#define VECOP( OP )  for( unsigned int i=0; i < DIM; i++ ) { OP }

#ifdef MATH_CLONE_VECTOR
#error MATH_CLONE_VECTOR already defined!
#endif
#define MATH_CLONE_VECTOR VECTOR ( static_cast<const VECTOR&>(*this) )


namespace Geometry {


///////////////////////////////////////////////////////////////////////////////
//! basic vector type
template<typename SCALAR, unsigned int DIM, typename VECTOR> 
class Vector
{
 protected:
  SCALAR m_v[DIM];

 public:

  //typedef Vector<SCALAR,DIM> VECTOR;

  explicit Vector() { VECOP( m_v[i]=0; ) }

  //Vector( const SCALAR* unsafe ) { VECOP( m_v[i]=unsafe[i]; ) }
	void set_( SCALAR* unsafe ) { VECOP( m_v[i]=unsafe[i]; ) }

  //! copy constructor
  Vector( const VECTOR& other ) { VECOP( m_v[i]=other[i]; ); }

  // copy assignment
  VECTOR& operator = ( const VECTOR& other )  { VECOP( m_v[i]=other[i]; ); return as_child(); }

  SCALAR* operator () ()
    {
      return m_v;
    };

  SCALAR operator [] ( const unsigned int comp ) const
    {
      assert( comp < DIM );
      return m_v[comp];
    };

  SCALAR& operator [] ( const unsigned int comp )
    {
      assert( (comp < DIM) );
      return m_v[comp];
    };


private:

  inline VECTOR& as_child() { return static_cast<VECTOR&>(*this); }
  inline const VECTOR& as_child() const { static_cast<const VECTOR&>(*this); }

public:

  inline VECTOR& operator *= (const SCALAR& s){VECOP( m_v[i] *= s;); return as_child(); }
  inline VECTOR& operator /= (const SCALAR& s){VECOP( m_v[i] /= s;); return as_child(); }
  inline VECTOR& operator += (const SCALAR& s){VECOP( m_v[i] += s;); return as_child(); }
  inline VECTOR& operator -= (const SCALAR& s){VECOP( m_v[i] -= s;); return as_child(); }

  inline VECTOR operator * (const SCALAR& s) const  {  return MATH_CLONE_VECTOR *= s;  }
  inline VECTOR operator / (const SCALAR& s) const  {  return MATH_CLONE_VECTOR /= s;  }
  inline VECTOR operator + (const SCALAR& s) const  {  return MATH_CLONE_VECTOR += s;  }
  inline VECTOR operator - (const SCALAR& s) const  {  return MATH_CLONE_VECTOR -= s;  }

  inline VECTOR& operator *= (const VECTOR& v){VECOP(m_v[i] *= v[i];); return as_child(); }
  inline VECTOR& operator /= (const VECTOR& v){VECOP(m_v[i] /= v[i];); return as_child(); }
  inline VECTOR& operator += (const VECTOR& v){VECOP(m_v[i] += v[i];); return as_child(); }
  inline VECTOR& operator -= (const VECTOR& v){VECOP(m_v[i] -= v[i];); return as_child(); }

  inline VECTOR operator * (const VECTOR& v) const  {  return MATH_CLONE_VECTOR *= v;  }
  inline VECTOR operator / (const VECTOR& v) const  {  return MATH_CLONE_VECTOR /= v;  }
  inline VECTOR operator + (const VECTOR& v) const  {  return MATH_CLONE_VECTOR += v;  }
  inline VECTOR operator - (const VECTOR& v) const  {  return MATH_CLONE_VECTOR -= v;  }

  inline bool operator == (const VECTOR& other) const 
  {
	  // TODO: implement "v==other +- epsilon" ?

	  //VECOP( if( !(m_v[i]==other[i]) ) return false; );
	  //return true;
	  bool ret=true;
	  VECOP( ret &= (m_v[i]==other[i]); );
	  return ret;
  }

  inline bool operator != (const VECTOR& other) const
  {
	  return !(*this == other);
  }

  //! scalar product
  inline SCALAR scalarprod( const VECTOR& u ) const
  {
	  SCALAR sp=0;
	  VECOP( sp += m_v[i]*u[i]; );
	  return sp;
  }

  //! unary minus
  inline VECTOR operator - ( void ) const
    {
      VECTOR v;
      VECOP( v[i] = -m_v[i]; );
      return v;
    };

  //! l2-norm
  inline SCALAR magnitude() const
    {
      SCALAR squaresum=0;
      VECOP( squaresum += m_v[i]*m_v[i]; );
      return sqrt( squaresum );
    };

  //! normalize
  inline void normalize()
    {
      SCALAR m = magnitude();
      if( fabs(m) > 0 )
      VECOP( m_v[i] /= m; );
    };
};


///////////////////////////////////////////////////////////////////////////////
// STREAM OPERATORS

template<typename SCALAR, unsigned int DIM, typename VECTOR>
std::istream& operator >> ( std::istream& is, Vector<SCALAR,DIM,VECTOR>& v )
{
  VECOP( is >> v[i]; );
  return is;
}

template<typename SCALAR, unsigned int DIM, typename VECTOR>
std::ostream& operator << ( std::ostream& os, Vector<SCALAR,DIM,VECTOR> v )
{
  VECOP( os << v[i] << " "; );
  return os;
}


///////////////////////////////////////////////////////////////////////////////
// SPECIALIZATIONS

//! 4d specialization of Vector
template<typename SCALAR>
class Vector4 : public Vector<SCALAR,4,Vector4<SCALAR> >
{
//	friend Vector4<SCALAR> operator * ( const Matrix4x4&, const Vector4<SCALAR>& );

public:
	SCALAR &x, &y, &z, &w,   &r, &g, &b, &a;

	Vector4() 
		: x(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  w(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3]), 
	      r(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  a(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3])
	{};

	Vector4( SCALAR x, SCALAR y, SCALAR z, SCALAR w )
		: x(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  w(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3]), 
	      r(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  a(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3])
	{
		(*this)[0]=x; (*this)[1]=y; (*this)[2]=z; (*this)[3]=w;
	};

	Vector4( const float v[4] )
		: x(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  w(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3]), 
	      r(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  a(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3])
	{
		(*this)[0]=v[0]; (*this)[1]=v[1]; (*this)[2]=v[2]; (*this)[3]=v[3];
	};

	//! copy constructor (must be specialized!)
	Vector4( const Vector4& other )
		: x(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  w(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3]),
	      r(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[2]),
		  a(Vector<SCALAR,4,Vector4<SCALAR> >::m_v[3])
	{
		if( &other != this )
		{
			(*this)[0] = other[0];
			(*this)[1] = other[1];
			(*this)[2] = other[2];
			(*this)[3] = other[3];
		}
	};
	
	Vector4& operator= ( const Vector4& other )
	{
		if( &other != this )
		{
			Vector<SCALAR,4,Vector4<SCALAR> >::operator= (other);
		}
		return *this;
	};
};

//! 2d specialization of Vector
template<typename SCALAR>
class Vector2 : public Vector<SCALAR,2,Vector2<SCALAR> >
{
public:
	SCALAR &x, &y;

	Vector2(): 
      x(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[0]),
      y(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[1]) 
	{};

	Vector2( SCALAR x, SCALAR y ):
      x(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[0]),
      y(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[1]) 
	{
		(*this)[0]=x; (*this)[1]=y;
	};
	
	//! copy constructor (must be specialized!)
	Vector2( const Vector2& other )
		: x(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,2,Vector2<SCALAR> >::m_v[1])
	{
		if( &other != this )
		{
			(*this)[0] = other[0];
			(*this)[1] = other[1];
		}
	};	

	Vector2& operator= ( const Vector2& other )
	{
		if( &other != this )
		{
			Vector<SCALAR,2,Vector2<SCALAR> >::operator= (other);
		}
		return *this;
	};
};

//! 3d specialization of Vector
template<typename SCALAR>
class Vector3 : public Vector<SCALAR,3,Vector3<SCALAR> >
{
public:
	SCALAR &x, &y, &z,  &r, &g, &b;

	Vector3()
		: x(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2]),
	      r(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2])
	{};

	Vector3( SCALAR x, SCALAR y, SCALAR z )
		: x(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2]),
	      r(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2])
	{
		(*this)[0]=x; (*this)[1]=y; (*this)[2]=z;
	};

	//! copy constructor (must be specialized!)
	Vector3( const Vector3& other )
		: x(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  y(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  z(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2]),
	      r(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[0]),
		  g(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[1]),
		  b(Vector<SCALAR,3,Vector3<SCALAR> >::m_v[2])
	{
		if( &other != this )
		{
			(*this)[0] = other[0];
			(*this)[1] = other[1];
			(*this)[2] = other[2];
		}
	};

	Vector3& operator= ( const Vector3& other )
	{
		if( &other != this )
		{
			return Vector<SCALAR,3,Vector3<SCALAR> >::operator= (other);
		}
		return *this;
	};

	//! cross product
	inline Vector3<SCALAR> cross( const Vector3<SCALAR> w ) const
	{
		//*this = Vector3<SCALAR>( y*w.z - z*w.y,
		//	                     z*w.x - x*w.z,
		//				         x*w.y - y*w.x );
		//return *this;
		return Vector3<SCALAR>( y*w.z - z*w.y,
		                        z*w.x - x*w.z,
								x*w.y - y*w.x );
	};

	////! cross product (operator %)
	//inline Vector3<SCALAR>& operator %= ( const Vector3<SCALAR> w )
	//{
	//	return cross( w );
	//};

	////! cross product (inline operator %)
	//inline Vector3<SCALAR> operator % ( const Vector3<SCALAR> w ) const
	//{
	//	return Vector3<SCALAR>(*this) %= w;
 //   };
};

#undef MATH_CLONE_VECTOR
#undef VECOP

///////////////////////////////////////////////////////////////////////////////
// Alternative typedefs
typedef Vector4<float> vec4;
typedef Vector3<float> vec3;
typedef Vector2<float> vec2;


}; // namespace Geometry

#endif
