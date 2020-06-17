/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <gtest/gtest.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <cmath>

#include <Tests/Printers.h>
#include <Tests/Matchers.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/PlayBackInfo.h>

#include <EMotionFX/Source/Transform.h>

#if defined(EMFX_SCALE_DISABLED)
#define EMFX_SCALE false
#else
#define EMFX_SCALE true
#endif

namespace EMotionFX
{
    static const float sqrt2 = std::sqrt(2.0f);
    static const float sqrt2over2 = sqrt2 / 2.0f;

    AZ::Matrix3x3 TensorProduct(const AZ::Vector3& u, const AZ::Vector3& v)
    {
        AZ::Matrix3x3 mat{};
        mat.SetElement(0, 0, u.GetX() * v.GetX());
        mat.SetElement(0, 1, u.GetX() * v.GetY());
        mat.SetElement(0, 2, u.GetX() * v.GetZ());
        mat.SetElement(1, 0, u.GetY() * v.GetX());
        mat.SetElement(1, 1, u.GetY() * v.GetY());
        mat.SetElement(1, 2, u.GetY() * v.GetZ());
        mat.SetElement(2, 0, u.GetZ() * v.GetX());
        mat.SetElement(2, 1, u.GetZ() * v.GetY());
        mat.SetElement(2, 2, u.GetZ() * v.GetZ());
        return mat;
    }

    TEST(TransformFixture, ConstructorNoArgs)
    {
        const Transform transform;
        EXPECT_TRUE(transform.mPosition.IsZero());
        EXPECT_EQ(transform.mRotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, ConstructFromVec3Quat)
    {
        const Transform transform(AZ::Vector3(6.0f, 7.0f, 8.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi));
        EXPECT_EQ(transform.mPosition, AZ::Vector3(6.0f, 7.0f, 8.0f));
        EXPECT_THAT(transform.mRotation, IsClose(AZ::Quaternion(sqrt2over2, 0.0f, 0.0f, sqrt2over2)));
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateOne());
        )
    }

    using TransformConstructFromVec3QuatVec3Params = ::testing::tuple<AZ::Vector3, ::testing::tuple<float, float, float>, AZ::Vector3>;
    class TransformConstructFromVec3QuatVec3Fixture
        : public ::testing::TestWithParam<TransformConstructFromVec3QuatVec3Params>
    {
    public:
        const AZ::Vector3& ExpectedPosition() const
        {
            return ::testing::get<0>(GetParam());
        }
        AZ::Quaternion ExpectedRotation() const
        {
            return MCore::AzEulerAnglesToAzQuat(
                ::testing::get<0>(::testing::get<1>(GetParam())),
                ::testing::get<1>(::testing::get<1>(GetParam())),
                ::testing::get<2>(::testing::get<1>(GetParam()))
            );
        }
        const AZ::Vector3& ExpectedScale() const
        {
            return ::testing::get<2>(GetParam());
        }

        bool HasNonUniformScale() const
        {
            const AZ::Vector3 scale = ExpectedScale();
            return !scale.GetX().IsClose(scale.GetY()) || !scale.GetX().IsClose(scale.GetZ()) || !scale.GetY().IsClose(scale.GetZ());
        }

        // Returns a transformation matrix where the position is mirrored, the
        // rotation axis is mirrored, and the rotation angle is negated
        AZ::Matrix4x4 GetMirroredTransform(const AZ::Vector3& axis) const
        {
            const AZ::Matrix3x3 mirrorMatrix = AZ::Matrix3x3::CreateIdentity() - (2.0f * TensorProduct(axis, axis));
            const AZ::Vector3 mirrorPosition = mirrorMatrix * ExpectedPosition();

            AZ::Vector3 extractedAxis;
            float extractedAngle;
            ExpectedRotation().ConvertToAxisAngle(extractedAxis, extractedAngle);
            const AZ::Quaternion mirrorRotation = AZ::Quaternion::CreateFromAxisAngleExact(
                mirrorMatrix * AZ::Vector3(extractedAxis.GetX(), extractedAxis.GetY(), extractedAxis.GetZ()),
                -extractedAngle
            );

            return AZ::Matrix4x4::CreateFromQuaternionAndTranslation(mirrorRotation, mirrorPosition)
                * AZ::Matrix4x4::CreateScale(ExpectedScale());
        }
    };

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, ConstructFromVec3QuatVec3)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        EXPECT_THAT(transform.mPosition, IsClose(ExpectedPosition()));
        EXPECT_THAT(transform.mRotation, IsClose(ExpectedRotation()));
        EMFX_SCALECODE
        (
            EXPECT_THAT(transform.mScale, IsClose(ExpectedScale()));
        )
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, SetFromVec3QuatVec3)
    {
        Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(8.0f, 9.0f, 10.0f));
        transform.Set(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        EXPECT_THAT(transform.mPosition, IsClose(ExpectedPosition()));
        EXPECT_THAT(transform.mRotation, IsClose(ExpectedRotation()));
        EMFX_SCALECODE
        (
            EXPECT_THAT(transform.mScale, IsClose(ExpectedScale()));
        )
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformConstructFromVec3QuatVec3Fixture,
        ::testing::Combine(
            ::testing::Values(
                AZ::Vector3::CreateZero(),
                AZ::Vector3(6.0f, 7.0f, 8.0f)
            ),
            ::testing::Combine(
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi),
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi),
                ::testing::Values(0.0f, AZ::Constants::QuarterPi, AZ::Constants::HalfPi)
            ),
            ::testing::Values(
                AZ::Vector3::CreateOne(),
                AZ::Vector3(2.0f, 2.0f, 2.0f),
                AZ::Vector3(2.0f, 3.0f, 4.0f)
            )
        )
    );

    TEST(TransformFixture, SetFromVec3Quat)
    {
        Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(8.0f, 9.0f, 10.0f));
        transform.Set(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi));
        EXPECT_EQ(transform.mPosition, AZ::Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(transform.mRotation, IsClose(AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi)));
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, Identity)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.Identity();
        EXPECT_EQ(transform.mPosition, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.mRotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateOne());
        )
    }

    TEST(TransformFixture, Zero)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.Zero();
        EXPECT_EQ(transform.mPosition, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.mRotation, AZ::Quaternion(0.0f, 0.0f, 0.0f, 0.0f));
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateZero());
        )
    }

    TEST(TransformFixture, ZeroWithIdentityQuaternion)
    {
        Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f), AZ::Vector3(4.0f, 5.0f, 6.0f));
        transform.ZeroWithIdentityQuaternion();
        EXPECT_EQ(transform.mPosition, AZ::Vector3::CreateZero());
        EXPECT_EQ(transform.mRotation, AZ::Quaternion::CreateIdentity());
        EMFX_SCALECODE
        (
            EXPECT_EQ(transform.mScale, AZ::Vector3::CreateZero());
        )
    }

    using TransformMultiplyParams = ::testing::tuple<Transform, Transform, Transform, Transform>;
    using TransformMultiplyFixture = ::testing::TestWithParam<TransformMultiplyParams>;

    TEST_P(TransformMultiplyFixture, Multiply)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());

        Transform multiply(inputA);
        multiply.Multiply(inputB);

        EXPECT_THAT(multiply, IsClose(expected));
    }

    TEST_P(TransformMultiplyFixture, Multiplied)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());
        EXPECT_THAT(
            inputA.Multiplied(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.Multiplied(Transform()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, PreMultiply)
    {
        Transform inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());
        EXPECT_THAT(
            inputA.PreMultiply(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.PreMultiply(Transform()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, MultiplyWithOutputParam)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());

        Transform output;
        inputA.Multiply(inputB, &output);

        EXPECT_THAT(output, IsClose(expected));
    }

    TEST_P(TransformMultiplyFixture, PreMultiplied)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());
        EXPECT_THAT(
            inputA.PreMultiplied(inputB),
            IsClose(expected)
        );
        EXPECT_THAT(
            inputA.PreMultiplied(Transform()),
            IsClose(inputA)
        );
    }

    TEST_P(TransformMultiplyFixture, PreMultiplyWithOutputParam)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<3>(GetParam());

        Transform output;
        inputA.PreMultiply(inputB, &output);

        EXPECT_THAT(
            output,
            IsClose(expected)
        );
    }

    TEST_P(TransformMultiplyFixture, operatorMult)
    {
        const Transform& inputA = ::testing::get<0>(GetParam());
        const Transform& inputB = ::testing::get<1>(GetParam());
        const Transform& expected = ::testing::get<2>(GetParam());
        const Transform& expectedPreMult = ::testing::get<3>(GetParam());

        EXPECT_THAT(
            inputA * inputB,
            IsClose(expected)
        );
        EXPECT_THAT(
            inputB * inputA,
            IsClose(expectedPreMult)
        );

        EXPECT_THAT(
            inputA * Transform(),
            IsClose(inputA)
        );
        EXPECT_THAT(
            inputB * Transform(),
            IsClose(inputB)
        );
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformMultiplyFixture,
        ::testing::Values(
            TransformMultiplyParams {
                /* input a */{},
                /* input b */{},
                /* a * b = */{},
                /* b * a = */{}
            },
            // symmetric cases (where a*b == b*a) -----------------------------
            TransformMultiplyParams {
                // just translation
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // just rotation
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // just scale
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation and rotation
                /* input a */{AZ::Vector3::CreateAxisY(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateAxisY(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* a * b = */{AZ::Vector3(0.0f, 1.0f + sqrt2over2, sqrt2over2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()},
                /* b * a = */{AZ::Vector3(0.0f, 1.0f + sqrt2over2, sqrt2over2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // rotation and scale
                /* input a */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            TransformMultiplyParams {
                // translation, rotation, and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                /* a * b = */{AZ::Vector3(3.0f, 1.0f, 1.0f + 2*sqrt2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)},
                /* b * a = */{AZ::Vector3(3.0f, 1.0f, 1.0f + 2*sqrt2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(4.0f, 4.0f, 4.0f)}
            },
            // asymmetric cases (where a*b != b*a) -----------------------------
            TransformMultiplyParams {
                // translation and rotation
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                // translate then rotate
                /* a * b = */{AZ::Vector3(1.0f, 0.0f, sqrt2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                // rotate then translate
                /* b * a = */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()}
            },
            TransformMultiplyParams {
                // translation and scale
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // translate then scale
                /* a * b = */{AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // scale then translate
                /* b * a = */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)}
            },
            TransformMultiplyParams {
                // rotation and scale
                // rotation * scale are only asymmetric when there is a translation involved as well
                /* input a */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                /* input b */{AZ::Vector3::CreateOne(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // rotate then scale
                /* a * b = */{AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                // scale then rotate
                /* b * a = */{AZ::Vector3(2.0f, 1.0f, 1.0f + sqrt2), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 2.0f, 2.0f)},
            }
        )
    );

    TEST(TransformFixture, TransformPoint)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .TransformPoint(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3(5.0f, 0.0f, 0.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .TransformPoint(AZ::Vector3::CreateAxisX()),
            IsClose(EMFX_SCALE ? AZ::Vector3(7.5f, 0.0f, 0.0f) : AZ::Vector3(6.0f, 0.0f, 0.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .TransformPoint(AZ::Vector3(0.0f, 1.0f, 0.0f)),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformPoint(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(1.0f, -sqrt2over2, sqrt2over2 * 5.0f))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformPoint(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(6.0f, 6.0f - sqrt2over2, 7.0f + sqrt2over2 * 5.0f))
        );
    }

    TEST(TransformFixture, TransformVector)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .TransformVector(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3::CreateZero())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .TransformVector(AZ::Vector3::CreateAxisX()),
            IsClose(EMFX_SCALE ? AZ::Vector3(2.5f, 0.0f, 0.0f) : AZ::Vector3::CreateAxisX())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .TransformVector(AZ::Vector3::CreateAxisY()),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3(1.0f, 2.0f, 3.0f))
                .TransformVector(AZ::Vector3::CreateOne()),
            IsClose(AZ::Vector3(1.0f, -sqrt2over2, sqrt2over2 * 5.0f))
        );
    }

    TEST(TransformFixture, RotateVector)
    {
        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity())
                .RotateVector(AZ::Vector3::CreateZero()),
            IsClose(AZ::Vector3::CreateZero())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.5f, 1.0f, 1.0f))
                .RotateVector(AZ::Vector3::CreateAxisX()),
            IsClose(AZ::Vector3::CreateAxisX())
        );

        EXPECT_THAT(
            Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateRotationX(AZ::Constants::QuarterPi), AZ::Vector3::CreateOne())
                .RotateVector(AZ::Vector3::CreateAxisY()),
            IsClose(AZ::Vector3(0.0f, sqrt2over2, sqrt2over2))
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Inverse)
    {
        // Inverse does not work properly when there is non-uniform scale
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        const Transform inverse = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Inverse();

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Inversed)
    {
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        const Transform inverse = transform.Inversed();

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CalcRelativeToWithOutputParam)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        const Transform someTransform(
            AZ::Vector3(20.0f, 30.0f, 40.0f),
            AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3(0.2f, 0.4f, 0.7f).GetNormalizedExact(), 0.25f),
            AZ::Vector3(2.0f, 3.0f, 4.0f)
        );

        Transform relative;
        transform.CalcRelativeTo(someTransform, &relative);

        EXPECT_THAT(
            relative * someTransform,
            IsClose(transform)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CalcRelativeTo)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        const Transform someTransform(
            AZ::Vector3(20.0f, 30.0f, 40.0f),
            AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3(0.2f, 0.4f, 0.7f).GetNormalizedExact(), 0.25f),
            AZ::Vector3(2.0f, 3.0f, 4.0f)
        );

        const Transform relative = transform.CalcRelativeTo(someTransform);

        EXPECT_THAT(
            relative * someTransform,
            IsClose(transform)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, InverseWithOutputParam)
    {
        if (HasNonUniformScale())
        {
            return;
        }

        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        Transform inverse;
        transform.Inverse(&inverse);

        const AZ::Vector3 point(1.0f, 2.0f, 3.0f);

        EXPECT_THAT(
            inverse.TransformPoint(transform.TransformPoint(point)),
            IsClose(point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Mirror)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        const Transform mirrorTransform = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirror(axis);

        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);

        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    TEST(TransformFixture, MirrorWithFlags)
    {
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, Mirrored)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        const Transform mirrorTransform = Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirrored(axis);

        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);

        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, MirrorWithOutputParam)
    {
        const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        Transform mirrorTransform;
        Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).Mirror(axis, &mirrorTransform);

        const AZ::Matrix4x4 mirrorMatrix = GetMirroredTransform(axis);

        const AZ::Vector3 point(3.0f, 4.0f, 5.0f);

        EXPECT_THAT(
            mirrorTransform.TransformPoint(point),
            IsClose(mirrorMatrix * point)
        );
    }

    struct ApplyDeltaParams
    {
        const Transform initial;
        const Transform a;
        const Transform b;
        const Transform expected;
        const float weight;
    };

    using TransformApplyDeltaFixture = ::testing::TestWithParam<ApplyDeltaParams>;

    TEST_P(TransformApplyDeltaFixture, ApplyDelta)
    {
        if (GetParam().weight != 1.0f)
        {
            return;
        }

        Transform transform = GetParam().initial;
        transform.ApplyDelta(GetParam().a, GetParam().b);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().expected)
        );
    }

    TEST_P(TransformApplyDeltaFixture, ApplyDeltaMirrored)
    {
        if (GetParam().weight != 1.0f)
        {
            return;
        }

        const AZ::Vector3 mirrorAxis = AZ::Vector3::CreateAxisX();

        Transform transform = GetParam().initial;
        transform.ApplyDeltaMirrored(GetParam().a, GetParam().b, mirrorAxis);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().expected.Mirrored(mirrorAxis))
        );
    }

    TEST_P(TransformApplyDeltaFixture, ApplyDeltaWithWeight)
    {
        Transform transform = GetParam().initial;
        transform.ApplyDeltaWithWeight(GetParam().a, GetParam().b, GetParam().weight);
        EXPECT_THAT(
            transform,
            IsClose(GetParam().expected)
        );
    }

    INSTANTIATE_TEST_CASE_P(Test, TransformApplyDeltaFixture,
        ::testing::ValuesIn(std::vector<ApplyDeltaParams>{
            {
                {},
                {AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(0.5f, 0.5f, 0.5f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                0.5f,
            },
            {
                {},
                {AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3(1.0f, 1.0f, 1.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                1.0f,
            },
            {
                {},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 4.0f), AZ::Vector3::CreateOne()},
                0.5f,
            },
            {
                {},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi / 2.0f), AZ::Vector3::CreateOne()},
                1.0f,
            },
            {
                {},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(1.5f, 1.5f, 1.5f)},
                0.5f,
            },
            {
                {},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                {AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3(2.0f, 2.0f, 2.0f)},
                1.0f,
            },
        })
    );

    TEST_P(TransformConstructFromVec3QuatVec3Fixture, CheckIfHasScale)
    {
        const Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());

        EXPECT_EQ(transform.CheckIfHasScale(), !ExpectedScale().IsClose(AZ::Vector3::CreateOne()));
    }

    TEST(TransformFixture, Normalize)
    {
        const Transform transform = Transform(
            AZ::Vector3::CreateOne(),
            AZ::Quaternion(2.0f, 0.0f, 0.0f, 2.0f),
            AZ::Vector3::CreateOne()
        ).Normalize();
        EXPECT_FLOAT_EQ(transform.mRotation.GetLengthExact(), 1.0f);
    }

    TEST(TransformFixture, Normalized)
    {
        const Transform transform = Transform(
            AZ::Vector3::CreateOne(),
            AZ::Quaternion(2.0f, 0.0f, 0.0f, 2.0f),
            AZ::Vector3::CreateOne()
        ).Normalized();
        EXPECT_FLOAT_EQ(transform.mRotation.GetLengthExact(), 1.0f);
    }

    TEST(TransformFixture, BlendAdditive)
    {
        {
            const Transform result =
                Transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3::CreateOne()).BlendAdditive(
                    /*dest=*/Transform(AZ::Vector3(11.0f, 12.0f, 13.0f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(2.0f, 2.0f, 2.0f)),
                    /*orgTransform=*/Transform(AZ::Vector3(8.0f, 10.0f, 12.0f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi), AZ::Vector3(2.0f, 3.0f, 2.0f)),
                    0.5f
                );

            EXPECT_THAT(
                result,
                IsClose(Transform(AZ::Vector3(6.5f, 7.0f, 7.5f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 3.0f / 8.0f), AZ::Vector3(1.0f, 0.5f, 1.0f)))
            );
        }
    }

    class TwoTransformsFixture
        : public ::testing::Test
    {
    protected:
        const AZ::Vector3 translationA{5.0f, 6.0f, 7.0f};
        const AZ::Quaternion rotationA = AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi);
        const AZ::Vector3 scaleA = AZ::Vector3::CreateOne();

        const AZ::Vector3 translationB{11.0f, 12.0f, 13.0f};
        const AZ::Quaternion rotationB = AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi);
        const AZ::Vector3 scaleB{3.0f, 4.0f, 5.0f};

    };

    TEST_F(TwoTransformsFixture, Blend)
    {
        const Transform transformA(translationA, rotationA, scaleA);
        const Transform transformB(translationB, rotationB, scaleB);

        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Blend(transformB, 0.0f),
            IsClose(transformA)
        );
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Blend(transformB, 0.25f),
            IsClose(Transform(AZ::Vector3(6.5f, 7.5f, 8.5f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 5.0f / 16.0f), AZ::Vector3(1.5f, 1.75f, 2.0f)))
        );
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Blend(transformB, 0.5f),
            IsClose(Transform(AZ::Vector3(8.0f, 9.0f, 10.0f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 3.0f / 8.0f), AZ::Vector3(2.0f, 2.5f, 3.0f)))
        );
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Blend(transformB, 0.75f),
            IsClose(Transform(AZ::Vector3(9.5f, 10.5f, 11.5f), AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisX(), AZ::Constants::Pi * 7.0f / 16.0f), AZ::Vector3(2.5f, 3.25f, 4.0f)))
        );
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Blend(transformB, 1.0f),
            IsClose(transformB)
        );
    }

    TEST_F(TwoTransformsFixture, ApplyAdditiveTransform)
    {
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).ApplyAdditive(Transform(translationB, rotationB, scaleB)),
            IsClose(Transform(translationA + translationB, rotationA * rotationB, scaleA * scaleB))
        );
    }

    TEST_F(TwoTransformsFixture, ApplyAdditiveTransformFloat)
    {
        const float factor = 0.5f;
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).ApplyAdditive(Transform(translationB, rotationB, scaleB), factor),
            IsClose(Transform(translationA + translationB * factor, rotationA.NLerp(rotationA * rotationB, factor), scaleA * AZ::Vector3::CreateOne().Lerp(scaleB, factor)))
        );
    }

    TEST_F(TwoTransformsFixture, AddTransform)
    {
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Add(Transform(translationB, rotationB, scaleB)),
            IsClose(Transform(translationA + translationB, rotationA + rotationB, scaleA + scaleB))
        );
    }

    TEST_F(TwoTransformsFixture, AddTransformFloat)
    {
        const float factor = 0.5f;
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Add(Transform(translationB, rotationB, scaleB), factor),
            IsClose(Transform(translationA + translationB * factor, rotationA + rotationB * factor, scaleA + scaleB * factor))
        );
    }

    TEST_F(TwoTransformsFixture, Subtract)
    {
        EXPECT_THAT(
            Transform(translationA, rotationA, scaleA).Subtract(Transform(translationB, rotationB, scaleB)),
            IsClose(Transform(translationA - translationB, rotationA - rotationB, scaleA - scaleB))
        );
    }

    class TransformProjectedToGroundPlaneFixture
        : public TransformConstructFromVec3QuatVec3Fixture
    {
    public:
        bool ShouldSkip() const
        {
            // These tests do not meet the expectation when there is both a
            // pitch and a roll value
            // This is because the combination of pitch + roll, even when yaw
            // is 0, introduces a rotation around z
            return ::testing::get<0>(::testing::get<1>(GetParam())) != 0
                && ::testing::get<1>(::testing::get<1>(GetParam())) != 0;
        }

        void Expect(const Transform& transform, float zValue) const
        {
            EXPECT_THAT(
                transform,
                IsClose(Transform(
                    AZ::Vector3(ExpectedPosition().GetX(), ExpectedPosition().GetY(), zValue),
                    AZ::Quaternion::CreateFromAxisAngleExact(AZ::Vector3::CreateAxisZ(), ::testing::get<2>(::testing::get<1>(GetParam()))),
                    ExpectedScale()
                ))
            );
        }
    };

    TEST_P(TransformProjectedToGroundPlaneFixture, ApplyMotionExtractionFlags)
    {
        if (ShouldSkip())
        {
            return;
        }
        Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        transform.ApplyMotionExtractionFlags(EMotionExtractionFlags(0));

        Expect(transform, 0.0f);
    }

    TEST_P(TransformProjectedToGroundPlaneFixture, ApplyMotionExtractionFlagsCaptureZ)
    {
        if (ShouldSkip())
        {
            return;
        }
        Transform transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale());
        transform.ApplyMotionExtractionFlags(MOTIONEXTRACT_CAPTURE_Z);

        Expect(transform, ExpectedPosition().GetZ());
    }

    TEST_P(TransformProjectedToGroundPlaneFixture, ProjectedToGroundPlane)
    {
        if (ShouldSkip())
        {
            return;
        }
        Expect(Transform(ExpectedPosition(), ExpectedRotation(), ExpectedScale()).ProjectedToGroundPlane(), 0.0f);
    }

    const auto possiblePitchAndYawValues = ::testing::Values(
        -AZ::Constants::HalfPi,
        -AZ::Constants::QuarterPi,
        0.0f,
        AZ::Constants::QuarterPi,
        AZ::Constants::HalfPi
    );
    INSTANTIATE_TEST_CASE_P(Test, TransformProjectedToGroundPlaneFixture,
        ::testing::Combine(
            ::testing::Values(
                AZ::Vector3::CreateZero(),
                AZ::Vector3(6.0f, 7.0f, 8.0f)
            ),
            ::testing::Combine(
                /* possible pitch values */ possiblePitchAndYawValues,
                /* possible roll values */ ::testing::Values(0.0f, AZ::Constants::QuarterPi),
                /* possible yaw values */ possiblePitchAndYawValues
            ),
            ::testing::Values(
                AZ::Vector3::CreateOne()
            )
        )
    );

} // namespace EMotionFX
