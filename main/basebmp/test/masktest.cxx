/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// autogenerated file with codegen.pl

#include "preextstl.h"
#include "gtest/gtest.h"
#include "postextstl.h"

#include <basegfx/vector/b2isize.hxx>
#include <basegfx/range/b2irange.hxx>
#include <basegfx/point/b2ipoint.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>

#include <basebmp/color.hxx>
#include <basebmp/scanlineformats.hxx>
#include <basebmp/bitmapdevice.hxx>
#include <basebmp/debug.hxx>
#include "tools.hxx"

#include <iostream>
#include <fstream>

using namespace ::basebmp;

namespace
{
/*
        std::ofstream output("32bpp_test.dump");
        debugDump( rDevice, output );
        std::ofstream output2("32bpp_bmp.dump");
        debugDump( rBmp, output2 );
*/

class MaskTest : public ::testing::Test
{
protected:
    BitmapDeviceSharedPtr mpDevice1bpp;
    BitmapDeviceSharedPtr mpDevice32bpp;
    BitmapDeviceSharedPtr mpMask;

    void implTestMaskBasics(const BitmapDeviceSharedPtr& rDevice,
                            const BitmapDeviceSharedPtr& rBmp)
    {
        const Color aCol(0);
        const Color aCol2(0xF0F0F0F0);

        const basegfx::B2IRange aSourceRect(0,0,10,10);
        const basegfx::B2IPoint aDestLeftTop(0,0);
        const basegfx::B2IPoint aDestRightTop(5,0);
        const basegfx::B2IPoint aDestLeftBottom(0,5);
        const basegfx::B2IPoint aDestRightBottom(5,5);

        rDevice->clear(aCol);
        rDevice->setPixel(
            basegfx::B2IPoint(1,1),
            aCol2,
            DrawMode_PAINT);
        rDevice->drawMaskedColor(
            aCol2,
            rBmp,
            aSourceRect,
            aDestLeftTop );
        ASSERT_TRUE(countPixel( rDevice, aCol ) == 100-50) << "number of rendered pixel is not 50";

        rDevice->clear(aCol);
        rDevice->drawMaskedColor(
            aCol2,
            rBmp,
            aSourceRect,
            aDestRightTop );
        ASSERT_TRUE(countPixel( rDevice, aCol ) == 100-25) << "number of rendered pixel is not 25";

        rDevice->clear(aCol);
        rDevice->drawMaskedColor(
            aCol2,
            rBmp,
            aSourceRect,
            aDestLeftBottom );
        ASSERT_TRUE(countPixel( rDevice, aCol ) == 100-25) << "number of rendered pixel is not 25(b)";

        rDevice->clear(aCol);
        rDevice->drawMaskedColor(
            aCol2,
            rBmp,
            aSourceRect,
            aDestRightBottom );
        ASSERT_TRUE(countPixel( rDevice, aCol ) == 100-25) << "number of rendered pixel is not 25(c)";
    }

public:
    virtual void SetUp()
    {
        const basegfx::B2ISize aSize(10,10);
        mpDevice1bpp = createBitmapDevice( aSize,
                                           true,
                                           Format::ONE_BIT_MSB_PAL );
        mpDevice32bpp = createBitmapDevice( aSize,
                                            true,
                                            Format::THIRTYTWO_BIT_TC_MASK );

        mpMask = createBitmapDevice( aSize,
                                     true,
                                     Format::EIGHT_BIT_GREY );

        ::rtl::OUString aSvg = ::rtl::OUString::createFromAscii(
            "m 0 0h5v10h5v-5h-10z" );

        basegfx::B2DPolyPolygon aPoly;
        basegfx::tools::importFromSvgD( aPoly, aSvg, false, NULL );
        const Color aCol(0xFF);
        mpMask->fillPolyPolygon(
            aPoly,
            aCol,
            DrawMode_PAINT );
    }
};

TEST_F(MaskTest, testMaskBasics)
{
    implTestMaskBasics( mpDevice32bpp, mpMask );
    implTestMaskBasics( mpDevice1bpp, mpMask );
}

TEST_F(MaskTest, testMaskClip)
{
}


}
