// __BEGIN_LICENSE__
// 
// Copyright (C) 2006 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration
// (NASA).  All Rights Reserved.
// 
// Copyright 2006 Carnegie Mellon University. All rights reserved.
// 
// This software is distributed under the NASA Open Source Agreement
// (NOSA), version 1.3.  The NOSA has been approved by the Open Source
// Initiative.  See the file COPYING at the top of the distribution
// directory tree for the complete NOSA document.
// 
// THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
// KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
// LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
// SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
// THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
// DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.
// 
// __END_LICENSE__

/// \file PixelMask.h
/// 
/// Defines the useful pixel utility type that can wrap any existing
/// pixel type and add mask semantics.  Any operations with an
/// "invalid" pixel returns an invalid pixel as a result.  
///
#ifndef __VW_IMAGE_PIXELMASK_H__
#define __VW_IMAGE_PIXELMASK_H__

#include <ostream>

#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/static_assert.hpp>

#include <vw/Image/ImageViewBase.h>
#include <vw/Image/PixelTypes.h>
#include <vw/Image/PixelTypeInfo.h>
#include <vw/Image/PixelMath.h>
#include <vw/Image/PixelAccessors.h>
#include <vw/Math/Vector.h>

namespace vw {

  // *******************************************************************
  // A pixel type convenience macro and forward declrations.
  // *******************************************************************

  /// This macro provides the appropriate specializations of the
  /// compound type traits classes for a new pixel type with a fixed
  /// number of channels (the common case).  This is a special
  /// adaptation that adds PixelMask<> semantics to any pixel type for
  /// which this macro has been executed.
#define VW_DECLARE_PIXEL_MASK_TYPE(PIXELT,NCHANNELS)                      \
  template <class ChannelT>                                               \
  struct CompoundChannelType<PixelMask<PIXELT<ChannelT> > > {             \
    typedef ChannelT type;                                                \
  };                                                                      \
  template <class ChannelT>                                               \
  struct CompoundNumChannels<PixelMask<PIXELT<ChannelT> > > {             \
    static const int32 value = NCHANNELS + 1;                             \
  };                                                                      \
  template <class OldChT, class NewChT>                                   \
  struct CompoundChannelCast<PixelMask<PIXELT<OldChT> >, NewChT> {        \
    typedef PixelMask<PIXELT<NewChT> > type;                              \
  };                                                                      \
  template <class OldChT, class NewChT>                                   \
  struct CompoundChannelCast<PixelMask<PIXELT<OldChT> >, const NewChT> {  \
    typedef const PixelMask<PIXELT<NewChT> > type;                        \
  }                                                                       \

  /// This macro provides the appropriate specializations of 
  /// the compound type traits classes for a new pixel type 
  /// with a variable number of channels.
#define VW_DECLARE_PIXEL_MASK_TYPE_NCHANNELS(PIXELT)                    \
  template <class ChannelT, int SizeN>                                   \
  struct CompoundChannelType<PixelMask<PIXELT<ChannelT,SizeN> > > {      \
    typedef ChannelT type;                                               \
  };                                                                     \
  template <class ChannelT, int SizeN>                                   \
  struct CompoundNumChannels<PixelMask<PIXELT<ChannelT,SizeN> > > {      \
    static const int32 value = SizeN + 1;                                \
  };                                                                     \
  template <class OldChT, class NewChT, int SizeN>                       \
  struct CompoundChannelCast<PixelMask<PIXELT<OldChT,SizeN> >, NewChT> { \
    typedef PixelMask<PIXELT<NewChT,SizeN> > type;                       \
  };                                                                     \
  template <class OldChT, class NewChT, int SizeN>                       \
  struct CompoundChannelCast<PixelMask<PIXELT<OldChT,SizeN> >, const NewChT> { \
    typedef const PixelMask<PIXELT<NewChT,SizeN> > type;                 \
  }

  // *******************************************************************
  // The PixelMask wrapper pixel type.
  // *******************************************************************

  /// A generic wrapper for any of the above pixel types that adds an
  /// additional "valid" bit to the pixel.  Math operations that
  /// include invalide pixels will produce resulting pixels that are
  /// themselves invalid.
  template <class ChildT>
  struct PixelMask : public PixelMathBase< PixelMask<ChildT> >
  {
    typedef typename CompoundChannelType<ChildT>::type channel_type;
    
  private:
    ChildT m_child;
    channel_type m_valid;

  public:
    typedef ChildT child_type;

    // Default constructor (zero value).  Pixel is not valid by
    // default.
    PixelMask() { 
      m_child = ChildT(); 
      m_valid = ChannelRange<channel_type>::min();
    }

    /// implicit construction from the raw channel value, or from the
    /// child pixel type value.  Values constructed in this manner
    /// are considered valid.
    template <class T>
    PixelMask( T const& pix) {
      m_child = pix;
      m_valid = ChannelRange<channel_type>::max();
    }

    /// Conversion from other PixelMask<> types.
    template <class OtherT> explicit PixelMask( PixelMask<OtherT> other ) {
      if (other.valid()) { 
        m_valid = ChannelRange<channel_type>::max();
        // We let the child's built-in conversions do their work here.
        // This will fail if there is no conversion defined from
        // OtherT to ChildT.
        m_child = ChildT(other.child());  
      } else {
        m_valid = channel_type();
        m_child = ChildT();
      }
    }

    /// Constructs a pixel with the given channel values (use only when child has 2 channels)
    PixelMask( channel_type const& a0, channel_type const& a1 ) {
      m_child[0]=a0; m_child[1]=a1; m_valid = ChannelRange<channel_type>::max();
    }

    /// Constructs a pixel with the given channel values (use only when child has 3 channels)
    PixelMask( channel_type const& a0, channel_type const& a1, channel_type const& a2 ) {
      m_child[0]=a0; m_child[1]=a1; m_child[2]=a2; m_valid = ChannelRange<channel_type>::max();
    }

    /// Constructs a pixel with the given channel values (use only when child has 4 channels)
    PixelMask( channel_type const& a0, channel_type const& a1, channel_type const& a2, channel_type const& a3 ) {
      m_child[0]=a0; m_child[1]=a1; m_child[2]=a2; m_child[3]=a3; m_valid = ChannelRange<channel_type>::max();
    }

    /// Returns the value from the valid channel
    channel_type valid() const { return m_valid; }

    /// Invalidates this pixel, setting its valid bit to zero.
    void invalidate() { m_valid = ChannelRange<channel_type>::min(); }

    /// Invalidates this pixel, setting its valid bit to 1;
    void validate() { m_valid = ChannelRange<channel_type>::max(); }

    /// Returns the child pixel type
    ChildT const& child() const { return m_child; }

    /// Automatic down-cast to the raw channel value in numeric
    /// contexts.  This should only work for pixels that contain one
    /// data channel (plus the mask channel).  We add a
    /// BOOST_STATIC_ASSERT here to make sure that this is the case.
    operator channel_type() const { 
       BOOST_STATIC_ASSERT(CompoundNumChannels<ChildT>::value == 1);
       return compound_select_channel<channel_type const&>(m_child,0); 
    }

    /// Channel indexing operator.
    inline channel_type& operator[](int i) { 
      if (i == CompoundNumChannels<ChildT>::value)
        return m_valid;
      else 
        return compound_select_channel<channel_type&>(m_child,i);
     }
    /// Channel indexing operator (const overload).
    inline channel_type const& operator[](int i) const { 
      if (i == CompoundNumChannels<ChildT>::value)
        return m_valid; 
      else 
        return compound_select_channel<channel_type const&>(m_child,i);
    }
    /// Channel indexing operator.
    inline channel_type& operator()(int i) { 
      if (i == CompoundNumChannels<ChildT>::value)
        return valid; 
      else
        return compound_select_channel<channel_type&>(m_child,i);
    }
    /// Channel indexing operator (const overload).
    inline channel_type const& operator()(int i) const { 
      if (i == CompoundNumChannels<ChildT>::value)
        return valid; 
      else
        return compound_select_channel<channel_type const&>(m_child,i);
    }
  };

  /// Print a PixelMask to a debugging stream.
  template <class ChildT>
  std::ostream& operator<<( std::ostream& os, PixelMask<ChildT> const& pix ) {
    return os << "PixelMask( " << _numeric(pix.valid()) << " " << pix.valid() << " )";
  }

  // We must declare each pixel type we wish to use with the valid pixel wrapper.
  VW_DECLARE_PIXEL_MASK_TYPE(PixelGray,1);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelGrayA,2);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelRGB,3);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelRGBA,4);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelHSV, 3);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelXYZ, 3);
  VW_DECLARE_PIXEL_MASK_TYPE(PixelLuv, 3);
  VW_DECLARE_PIXEL_MASK_TYPE_NCHANNELS(Vector);

  // Computes the mean value of a compound PixelMask<> type.  Not
  // especially efficient.
  template <class T>
  typename boost::enable_if< IsScalarOrCompound<T>, double >::type
  inline mean_channel_value( PixelMask<T> const& arg ) {
    typedef typename CompoundChannelType<T>::type channel_type;
    if (arg.valid()) {
      int num_channels = CompoundNumChannels<T>::value;
      double accum = 0;
      for( int i=0; i<num_channels-1; ++i )
        accum += compound_select_channel<channel_type const&>( arg, i );
      return accum / num_channels;
    } else {
      return 0;
    }
  }

  // Overload for the pixel transparency traits class.  
  template <class ChildT>
  bool is_transparent(PixelMask<ChildT> const& pixel) { return !pixel.valid(); }

  // *******************************************************************
  // Binary elementwise compound type functor.
  // *******************************************************************

  template <class FuncT>
  class PixelMaskBinaryCompoundFunctor {
    FuncT func;

    // The general multi-channel case
    template <bool CompoundB, int ChannelsN, class ResultT, class Arg1T, class Arg2T>
    struct Helper {
      static inline ResultT construct( FuncT const& func, Arg1T const& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid()) {
          ResultT result;
          for( int i=0; i<ChannelsN-1; ++i ) result[i] = func(arg1[i],arg2[i]);
          result.validate();
          return result;
        } else {
          return ResultT();
        }
      }
    };

    // Specialization for one-channel + 1 "valid pixel" channel
    template <class ResultT, class Arg1T, class Arg2T>
    struct Helper<true,2,ResultT,Arg1T,Arg2T> {
      static inline ResultT construct( FuncT const& func, Arg1T const& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid())
          return ResultT( func(arg1[0],arg2[0]) );
        else 
          return ResultT();
      }
    };

    // Specialization for two-channel + 1 valid pixel channel types
    template <class ResultT, class Arg1T, class Arg2T>
    struct Helper<true,3,ResultT,Arg1T,Arg2T> {
      static inline ResultT construct( FuncT const& func, Arg1T const& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid())
          return ResultT( func(arg1[0],arg2[0]), func(arg1[1],arg2[1]) );
        else 
          return ResultT();
      }
    };

    // Specialization for three-channel + 1 valid pixel channel types
    template <class ResultT, class Arg1T, class Arg2T>
    struct Helper<true,4,ResultT,Arg1T,Arg2T> {
      static inline ResultT construct( FuncT const& func, Arg1T const& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid())
          return ResultT( func(arg1[0],arg2[0]), func(arg1[1],arg2[1]), func(arg1[2],arg2[2]) );
        else 
          return ResultT();
      }
    };

    // Specialization for four-channel + 1 valid pixel channel types
    template <class ResultT, class Arg1T, class Arg2T>
    struct Helper<true,5,ResultT,Arg1T,Arg2T> {
      static inline ResultT construct( FuncT const& func, Arg1T const& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid())
          return ResultT( func(arg1[0],arg2[0]), func(arg1[1],arg2[1]), func(arg1[2],arg2[2]), func(arg1[3],arg2[3]) );
        else 
          return ResultT();
      }
    };

  public:
    PixelMaskBinaryCompoundFunctor() : func() {}
    PixelMaskBinaryCompoundFunctor( FuncT const& func ) : func(func) {}
    
    template <class ArgsT> struct result {};

    template <class F, class Arg1T, class Arg2T>
    struct result<F(Arg1T,Arg2T)> {
      typedef typename CompoundChannelType<Arg1T>::type arg1_type;
      typedef typename CompoundChannelType<Arg2T>::type arg2_type;
      typedef typename boost::result_of<FuncT(arg1_type,arg2_type)>::type result_type;
      typedef typename CompoundChannelCast<Arg1T,result_type>::type type;
    };

    template <class Arg1T, class Arg2T>
    typename result<PixelMaskBinaryCompoundFunctor(Arg1T,Arg2T)>::type
    inline operator()( Arg1T const& arg1, Arg2T const& arg2 ) const {
      typedef typename result<PixelMaskBinaryCompoundFunctor(Arg1T,Arg2T)>::type result_type;
      return Helper<IsCompound<result_type>::value,CompoundNumChannels<result_type>::value,result_type,Arg1T,Arg2T>::construct(func,arg1,arg2);
    }
  };


  template <class FuncT, class Arg1T, class Arg2T=void>
  struct PixelMaskCompoundResult {
    typedef typename boost::result_of<PixelMaskBinaryCompoundFunctor<FuncT>(Arg1T,Arg2T)>::type type;
  };

  template <class FuncT, class Arg1T, class Arg2T>
  typename PixelMaskCompoundResult<FuncT,PixelMask<Arg1T>,PixelMask<Arg2T> >::type
  inline compound_apply( FuncT const& func, PixelMask<Arg1T> const& arg1, PixelMask<Arg2T> const& arg2 ) {
    return PixelMaskBinaryCompoundFunctor<FuncT>(func)(arg1,arg2);
  }


  // *******************************************************************
  // Binary in-place elementwise compound type functor.
  // *******************************************************************

  template <class FuncT>
  class PixelMaskBinaryInPlaceCompoundFunctor {
    FuncT func;

    // The general multi-channel case
    template <bool CompoundB, int ChannelsN, class Arg1T, class Arg2T>
    struct Helper {
      static inline Arg1T& apply( FuncT const& func, Arg1T& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid()) {
          for( int i=0; i<ChannelsN-1; ++i ) func(arg1[i],arg2[i]);
        } else {
          arg1 = Arg1T();
        }
        return arg1;
      }
    };

    // Specialization for one-channel types + 1 "valid pixel" channel
    template <class Arg1T, class Arg2T>
    struct Helper<true,2,Arg1T,Arg2T> {
      static inline Arg1T& apply( FuncT const& func, Arg1T& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid())
          func(arg1[0],arg2[0]);
        else 
          arg1 = Arg1T();
        return arg1;
      }
    };

    // Specialization for two-channel types + 1 "valid pixel" channel
    template <class Arg1T, class Arg2T>
    struct Helper<true,3,Arg1T,Arg2T> {
      static inline Arg1T& apply( FuncT const& func, Arg1T& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid()) {
          func(arg1[0],arg2[0]);
          func(arg1[1],arg2[1]);
        } else {
          arg1 = Arg1T();
        }
        return arg1;
      }
    };

    // Specialization for three-channel types + 1 "valid pixel" channel
    template <class Arg1T, class Arg2T>
    struct Helper<true,4,Arg1T,Arg2T> {
      static inline Arg1T& apply( FuncT const& func, Arg1T& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid()) {
          func(arg1[0],arg2[0]);
          func(arg1[1],arg2[1]);
          func(arg1[2],arg2[2]);
        } else {
          arg1 = Arg1T();
        }
        return arg1;
      }
    };

    // Specialization for four-channel types + 1 "valid pixel" channel
    template <class Arg1T, class Arg2T>
    struct Helper<true,5,Arg1T,Arg2T> {
      static inline Arg1T& apply( FuncT const& func, Arg1T& arg1, Arg2T const& arg2 ) {
        if (arg1.valid() && arg2.valid()) {
          func(arg1[0],arg2[0]);
          func(arg1[1],arg2[1]);
          func(arg1[2],arg2[2]);
          func(arg1[3],arg2[3]);
        } else {
          arg1 = Arg1T();
        }
        return arg1;
      }
    };

  public:
    PixelMaskBinaryInPlaceCompoundFunctor() : func() {}
    PixelMaskBinaryInPlaceCompoundFunctor( FuncT const& func ) : func(func) {}
    
    template <class ArgsT> struct result {};

    template <class F, class Arg1T, class Arg2T>
    struct result<F(Arg1T,Arg2T)> {
      typedef Arg1T& type;
    };

    template <class Arg1T, class Arg2T>
    typename result<PixelMaskBinaryInPlaceCompoundFunctor(Arg1T,Arg2T)>::type
    inline operator()( Arg1T& arg1, Arg2T const& arg2 ) const {
      return Helper<IsCompound<Arg1T>::value,CompoundNumChannels<Arg1T>::value,Arg1T,Arg2T>::apply(func,arg1,arg2);
    }
  };

  template <class FuncT, class Arg1T, class Arg2T>
  inline PixelMask<Arg1T>& compound_apply_in_place( FuncT const& func, PixelMask<Arg1T>& arg1, PixelMask<Arg2T> const& arg2 ) {
    return PixelMaskBinaryInPlaceCompoundFunctor<FuncT>(func)(arg1,arg2);
  }


  // *******************************************************************
  // Unary elementwise compound type functor.
  // *******************************************************************

  template <class FuncT>
  class PixelMaskUnaryCompoundFunctor {
    FuncT func;

    // The general multi-channel case
    template <bool CompoundB, int ChannelsN, class ResultT, class ArgT>
    struct Helper {
      static inline ResultT construct( FuncT const& func, ArgT const& arg ) {
        if (arg.valid()) {
          ResultT result;
          for( int i=0; i<ChannelsN-1; ++i ) result[i] = func(arg[i]);
          result.validate();
          return result;
        } else {
          return ResultT();
        }
      }
    };

    // Specialization for one-channel + 1 "valid pixel" channel
    template <class ResultT, class ArgT>
    struct Helper<true,2,ResultT,ArgT> {
      static inline ResultT construct( FuncT const& func, ArgT const& arg ) {
        if (arg.valid())
          return ResultT( func(arg[0]) );
        else 
          return ResultT();
      }
    };

    // Specialization for two-channel + 1 valid pixel channel types
    template <class ResultT, class ArgT>
    struct Helper<true,3,ResultT,ArgT> {
      static inline ResultT construct( FuncT const& func, ArgT const& arg ) {
        if (arg.valid())
          return ResultT( func(arg[0]), func(arg[1]) );
        else 
          return ResultT();
      }
    };

    // Specialization for three-channel + 1 valid pixel channel types
    template <class ResultT, class ArgT>
    struct Helper<true,4,ResultT,ArgT> {
      static inline ResultT construct( FuncT const& func, ArgT const& arg ) {
        if (arg.valid())
          return ResultT( func(arg[0]), func(arg[1]), func(arg[2]) );
        else 
          return ResultT();
      }
    };

    // Specialization for four-channel + 1 valid pixel channel types
    template <class ResultT, class ArgT>
    struct Helper<true,5,ResultT,ArgT> {
      static inline ResultT construct( FuncT const& func, ArgT const& arg ) {
        if (arg.valid())
          return ResultT( func(arg[0]), func(arg[1]), func(arg[2]), func(arg[3]) );
        else 
          return ResultT();
      }
    };

  public:
    PixelMaskUnaryCompoundFunctor() : func() {}
    PixelMaskUnaryCompoundFunctor( FuncT const& func ) : func(func) {}
    
    template <class ArgsT> struct result {};

    template <class F, class ArgT>
    struct result<F(ArgT)> {
      typedef typename CompoundChannelType<ArgT>::type arg_type;
      typedef typename boost::result_of<FuncT(arg_type)>::type result_type;
      typedef typename CompoundChannelCast<ArgT,result_type>::type type;
    };

    template <class ArgT>
    typename result<PixelMaskUnaryCompoundFunctor(ArgT)>::type
    inline operator()( ArgT const& arg ) const {
      typedef typename result<PixelMaskUnaryCompoundFunctor(ArgT)>::type result_type;
      return Helper<IsCompound<result_type>::value,CompoundNumChannels<result_type>::value,result_type,ArgT>::construct(func,arg);
    }
  };

  template <class FuncT, class ArgT>
  struct PixelMaskCompoundResult<FuncT,ArgT,void> {
    typedef typename boost::result_of<PixelMaskUnaryCompoundFunctor<FuncT>(ArgT)>::type type;
  };

  template <class FuncT, class ArgT>
  typename PixelMaskCompoundResult<FuncT,PixelMask<ArgT> >::type
  inline compound_apply( FuncT const& func, PixelMask<ArgT> const& arg ) {
    return PixelMaskUnaryCompoundFunctor<FuncT>(func)(arg);
  }


  // *******************************************************************
  // Unary in-place elementwise compound type functor.
  // *******************************************************************

  template <class FuncT>
  class PixelMaskUnaryInPlaceCompoundFunctor {
    FuncT func;
    typedef typename boost::add_reference<FuncT>::type func_ref;

    // The general multi-channel case
    template <bool CompoundB, int ChannelsN, class ArgT>
    struct Helper {
      static inline ArgT& apply( func_ref func, ArgT& arg ) {
        if (arg.valid()) {
          for( int i=0; i<ChannelsN-1; ++i ) func(arg[i]);
        } else {
          arg = ArgT();
        }
        return arg;
      }
    };

    // Specialization for one-channel types + 1 "valid pixel" channel
    template <class ArgT>
    struct Helper<true,2,ArgT> {
      static inline ArgT& apply( func_ref func, ArgT& arg ) {
        if (arg.valid())
          func(arg[0]);
        else 
          arg = ArgT();
        return arg;
      }
    };

    // Specialization for two-channel types + 1 "valid pixel" channel
    template <class ArgT>
    struct Helper<true,3,ArgT> {
      static inline ArgT& apply( func_ref func, ArgT& arg ) {
        if (arg.valid()) {
          func(arg[0]);
          func(arg[1]);
        } else {
          arg = ArgT();
        }
        return arg;
      }
    };

    // Specialization for three-channel types + 1 "valid pixel" channel
    template <class ArgT>
    struct Helper<true,4,ArgT> {
      static inline ArgT& apply( func_ref func, ArgT& arg ) {
        if (arg.valid()) {
          func(arg[0]);
          func(arg[1]);
          func(arg[2]);
        } else {
          arg = ArgT();
        }
        return arg;
      }
    };

    // Specialization for four-channel types + 1 "valid pixel" channel
    template <class ArgT>
    struct Helper<true,5,ArgT> {
      static inline ArgT& apply( func_ref func, ArgT& arg ) {
        if (arg.valid()) {
          func(arg[0]);
          func(arg[1]);
          func(arg[2]);
          func(arg[3]);
        } else {
          arg = ArgT();
        }
        return arg;
      }
    };

  public:
    PixelMaskUnaryInPlaceCompoundFunctor() : func() {}
    PixelMaskUnaryInPlaceCompoundFunctor( func_ref func ) : func(func) {}
    
    template <class ArgsT> struct result {};

    /// FIXME: This seems not to respect the constness of ArgT?  Weird?
    template <class F, class ArgT>
    struct result<F(ArgT)> {
      typedef ArgT& type;
    };

    template <class ArgT>
    inline ArgT& operator()( ArgT& arg ) const {
      return Helper<IsCompound<ArgT>::value,CompoundNumChannels<ArgT>::value,ArgT>::apply(func,arg);
    }

    template <class ArgT>
    inline const ArgT& operator()( const ArgT& arg ) const {
      return Helper<IsCompound<ArgT>::value,CompoundNumChannels<ArgT>::value,const ArgT>::apply(func,arg);
    }
  };

  template <class FuncT, class ArgT>
  inline PixelMask<ArgT>& compound_apply_in_place( FuncT& func, PixelMask<ArgT>& arg ) {
    return PixelMaskUnaryInPlaceCompoundFunctor<FuncT&>(func)(arg);
  }

  template <class FuncT, class ArgT>
  inline const PixelMask<ArgT>& compound_apply_in_place( FuncT& func, PixelMask<ArgT> const& arg ) {
    return PixelMaskUnaryInPlaceCompoundFunctor<FuncT&>(func)(arg);
  }

  template <class FuncT, class ArgT>
  inline PixelMask<ArgT>& compound_apply_in_place( FuncT const& func, PixelMask<ArgT>& arg ) {
    return PixelMaskUnaryInPlaceCompoundFunctor<FuncT const&>(func)(arg);
  }

  template <class FuncT, class ArgT>
  inline const PixelMask<ArgT>& compound_apply_in_place( FuncT const& func, PixelMask<ArgT> const& arg ) {
    return PixelMaskUnaryInPlaceCompoundFunctor<FuncT const&>(func)(arg);
  }

  // *******************************************************************
  /// MaskView
  ///
  /// Given a view with pixels of type PixelT and a pixel value to
  /// consider as the "no data" or masked value, returns a view with
  /// pixels that are of the PixelMask<PixelT>, with the appropriate
  /// pixels masked.
  ///
  template <class ViewT>
  class CreatePixelMaskView : public ImageViewBase<CreatePixelMaskView<ViewT> > {
    ViewT m_view;
    typename ViewT::pixel_type m_nodata_value;
    bool m_use_nodata_value;

  public:
    typedef PixelMask<typename ViewT::pixel_type> pixel_type;
    typedef PixelMask<typename ViewT::pixel_type> const result_type;
    typedef ProceduralPixelAccessor<CreatePixelMaskView> pixel_accessor;

    CreatePixelMaskView( ViewT const& view, typename ViewT::pixel_type const& nodata_value) 
      : m_view(view), m_use_nodata_value(false) {}

    void set_nodata_value( typename ViewT::pixel_type value ) {
      m_nodata_value = value;
      m_use_nodata_value = true;
    }

    inline int32 cols() const { return m_view.cols(); }
    inline int32 rows() const { return m_view.rows(); }
    inline int32 planes() const { return m_view.planes(); }

    inline pixel_accessor origin() const { return pixel_accessor( *this ); }

    inline result_type operator()( int32 col, int32 row, int32 plane=0 ) const { 
      if (m_use_nodata_value && m_view(col,row,plane) == m_nodata_value) {
        pixel_type result = m_view(col,row,plane);
        result.invalidate();
        return result;
      } else {
        return m_view(col,row,plane);
      }
    }
    
    typedef CreatePixelMaskView prerasterize_type;
    inline prerasterize_type prerasterize( BBox2i /*bbox*/ ) const { return *this; }
    template <class DestT> inline void rasterize( DestT const& dest, BBox2i bbox ) const {
      vw::rasterize( prerasterize(bbox), dest, bbox );
    }
  };

  template <class ViewT>
  struct IsMultiplyAccessible<CreatePixelMaskView<ViewT> > : public true_type {};

  template <class ViewT>
  CreatePixelMaskView<ViewT> create_mask( ImageViewBase<ViewT> const& view, typename ViewT::pixel_type const& value) {
    CreatePixelMaskView<ViewT> pm_view( view.impl(), value );
    pm_view.set_nodata_value(value);
    return pm_view;
  }

  template <class ViewT>
  CreatePixelMaskView<ViewT> create_mask( ImageViewBase<ViewT> const& view) {
    return CreatePixelMaskView<ViewT>( view.impl() );
  }

  // *******************************************************************
  /// ApplyPixelMaskView
  ///
  /// Given a view with pixels of the type PixelMask<T>, this view
  /// returns an image with pixels of type T where any pixel that was
  /// marked as "invalid" in the mask is replaced with the constant
  /// pixel value passed in as replacement_value.  The
  /// replacement_value is T() by default.
  ///
  template <class ViewT>
  class ApplyPixelMaskView : public ImageViewBase<ApplyPixelMaskView<ViewT> > {
    ViewT m_view;
    typename ViewT::pixel_type::child_type m_replacement_value;

  public:
    typedef typename ViewT::pixel_type::child_type pixel_type;
    typedef typename ViewT::pixel_type::child_type const result_type;
    typedef ProceduralPixelAccessor<ApplyPixelMaskView> pixel_accessor;

    ApplyPixelMaskView( ViewT const& view, pixel_type replacement_value = pixel_type() ) : m_view(view) {}

    inline int32 cols() const { return m_view.cols(); }
    inline int32 rows() const { return m_view.rows(); }
    inline int32 planes() const { return m_view.planes(); }

    inline pixel_accessor origin() const { return pixel_accessor( *this ); }

    inline result_type operator()( int32 col, int32 row, int32 plane=0 ) const { 
      typename ViewT::pixel_type& px = m_view(col,row,plane);
      if ( !is_transparent(px) ) {
        return px.child();
      } else {
        return m_replacement_value;
      }
    }

    typedef ApplyPixelMaskView prerasterize_type;
    inline prerasterize_type prerasterize( BBox2i /*bbox*/ ) const { return *this; }
    template <class DestT> inline void rasterize( DestT const& dest, BBox2i bbox ) const {
      vw::rasterize( prerasterize(bbox), dest, bbox );
    }
  };

  template <class ViewT>
  struct IsMultiplyAccessible<ApplyPixelMaskView<ViewT> > : public true_type {};

  template <class ViewT>
  ApplyPixelMaskView<ViewT> apply_mask( ImageViewBase<ViewT> const& view, 
                                        typename ViewT::pixel_type::child_type const& value = 
                                        typename ViewT::pixel_type::child_type() ) {
    return ApplyPixelMaskView<ViewT>( view.impl(), value );
  }

}

#endif // __VW_IMAGE_PIXELMASK_H__