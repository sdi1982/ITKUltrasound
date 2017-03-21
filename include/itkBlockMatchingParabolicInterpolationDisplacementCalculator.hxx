/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkBlockMatchingParabolicInterpolationDisplacementCalculator_hxx
#define itkBlockMatchingParabolicInterpolationDisplacementCalculator_hxx

#include "itkBlockMatchingParabolicInterpolationDisplacementCalculator.h"

#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"

namespace itk
{
namespace BlockMatching
{

template < class TMetricImage, class TDisplacementImage, class TCoordRep >
ParabolicInterpolationDisplacementCalculator< TMetricImage, TDisplacementImage,
                                              TCoordRep >
::ParabolicInterpolationDisplacementCalculator()
{
  this->m_CacheMetricImage = true;
}


template < class TMetricImage, class TDisplacementImage, class TCoordRep >
ITK_THREAD_RETURN_TYPE
ParabolicInterpolationDisplacementCalculator< TMetricImage, TDisplacementImage, TCoordRep >
::ParabolicInterpolationThreadFunctor::operator() ( Superclass *superclass,
                                                    RegionType& region, ThreadIdType threadId )
{
  Self* self = dynamic_cast< Self* >( superclass );

  // Find index of the maximum value.
  PixelType max = NumericTraits< PixelType >::min();
  IndexType maxIndex;
  maxIndex.Fill( 0 );

  TCoordRep y1 = max;
  TCoordRep y0;
  TCoordRep y2;

  PointType    maxPoint;
  SpacingType  spacing;
  IndexType    tempIndex = maxIndex;
  RegionType   metricImageRegion;

  MetricImageImageIteratorType imageImageIt( self->m_MetricImageImage,
    region );
  ImageRegionConstIterator< CenterPointsImageType > centerPointsIt( self->m_CenterPointsImage, region );
  ImageRegionIterator< DisplacementImageType > displacementIt( self->m_DisplacementImage, region );
  MetricImagePointerType metricImage;
  for( imageImageIt.GoToBegin(), centerPointsIt.GoToBegin(), displacementIt.GoToBegin();
       !imageImageIt.IsAtEnd();
       ++imageImageIt, ++centerPointsIt, ++displacementIt )
    {
    metricImage = imageImageIt.Get();
    spacing = metricImage->GetSpacing();

    // Find the max sampled location.
    max = NumericTraits< PixelType >::min();
    maxIndex.Fill( 0 );
    metricImageRegion = metricImage->GetBufferedRegion();
    ImageRegionConstIteratorWithIndex< MetricImageType > it( metricImage, metricImageRegion );
    for( it.GoToBegin(); !it.IsAtEnd(); ++it )
      {
      if( it.Get() > max )
        {
        max = it.Get();
        maxIndex = it.GetIndex();
        }
      }

    y1 = max;
    metricImage->TransformIndexToPhysicalPoint( maxIndex, maxPoint );
    tempIndex = maxIndex;
    for( unsigned int i = 0; i < ImageDimension; i++ )
      {
      tempIndex[i] = maxIndex[i] - 1;
      if( ! metricImageRegion.IsInside( tempIndex ) )
        {
        tempIndex[i] = maxIndex[i];
        continue;
        }
      y0 = metricImage->GetPixel( tempIndex );
      tempIndex[i] = maxIndex[i] + 1;
      if( ! metricImageRegion.IsInside( tempIndex ) )
        {
        tempIndex[i] = maxIndex[i];
        continue;
        }
      y2 = metricImage->GetPixel( tempIndex );
      tempIndex[i] = maxIndex[i];

      // delta = spacing * ( y0 - y2 ) / 2* ( y 0 - 2 y1 + y2 )
      maxPoint[i] += spacing[i] * ( y0 - y2 ) / ( 2 * ( y0 - 2 * y1 + y2 ) );
      }

    displacementIt.Set( maxPoint - centerPointsIt.Get() );
    }

  return ITK_THREAD_RETURN_VALUE;
}


template < class TMetricImage, class TDisplacementImage, class TCoordRep >
void
ParabolicInterpolationDisplacementCalculator< TMetricImage, TDisplacementImage,
                                              TCoordRep >
::Compute()
{
  this->m_Threader->SetNumberOfThreads( this->GetNumberOfThreads() );
  ApplyThreadFunctor( m_ParabolicInterpolationThreadFunctor );
  this->m_DisplacementImage->Modified();
}

} // end namespace BlockMatching
} // end namespace itk

#endif
