#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov8850raw.h"
#include "camera_info_ov8850raw.h"
#include "camera_custom_AEPlinetable.h"
const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,
    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 1819238756, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    },
    ISPPca:{
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
        },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
        }
    }},
    ISPCcmPoly22:{
        68540,    // i4R_AVG
        17761,    // i4R_STD
        100580,    // i4B_AVG
        25101,    // i4B_STD
        {  // i4P00[9]
            5396000, -2840000, 0, -1038000, 4130000, -532000, -224000, -2836000, 5622000
        },
        {  // i4P10[9]
            1048131, -1512486, 475501, 180820, -335017, 154196, -25667, -438377, 466863
        },
        {  // i4P01[9]
            254786, -905140, 660126, 131400, -732935, 601535, -180836, -1523366, 1711845
        },
        {  // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P02[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1152,    // u4MinGain, 1024 base = 1x
            8000,    // u4MaxGain, 16x
            75,    // u4MiniISOGain, ISOxx
            64,    // u4GainStepUnit, 1x/8
            27,    // u4PreExpUnit
            30,    // u4PreMaxFrameRate
            27,    // u4VideoExpUnit
            30,    // u4VideoMaxFrameRate
            1024,    // u4Video2PreRatio, 1024 base = 1x
            27,    // u4CapExpUnit
            14,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            24,    // u4LensFno, Fno = 2.8
            237    // u4FocusLength_100x
        },
        // rHistConfig
        {
            3, //4,    // u4HistHighThres
            40,    // u4HistLowThres
            1,  //2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {75, 108, 138, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM]
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            TRUE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            FALSE,    // bEnableCaptureThres
            FALSE,    // bEnableVideoThres
            FALSE,    // bEnableStrobeThres
            55,    // u4AETarget
            50,    // u4StrobeAETarget
            20, //50,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            2,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,    // u4InDoorEV = 9.0, 10 base
            -8, //-10,    // i4BVOffset delta BV = value/10
            70, //64,    // u4PreviewFlareOffset
            70,//64,    // u4CaptureFlareOffset
            3,    // u4CaptureFlareThres
            70, //64,    // u4VideoFlareOffset
            3,    // u4VideoFlareThres
            70,//64,    // u4StrobeFlareOffset
            3,    // u4StrobeFlareThres
            160,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            160,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            64 //75    // u4FlatnessStrength
        }
    },
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                956,    // i4R
                512,    // i4G
                629    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                91,    // i4X
                -276    // i4Y
            },
            // Horizon
            {
                -375,    // i4X
                -324    // i4Y
            },
            // A
            {
                -259,    // i4X
                -339    // i4Y
            },
            // TL84
            {
                -96,    // i4X
                -314    // i4Y
            },
            // CWF
            {
                -73,    // i4X
                -380    // i4Y
            },
            // DNP
            {
                -6,    // i4X
                -369    // i4Y
            },
            // D65
            {
                155,    // i4X
                -307    // i4Y
            },
            // DF
            {
                100,    // i4X
                -306    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                77,    // i4X
                -281    // i4Y
            },
            // Horizon
            {
                -391,    // i4X
                -305    // i4Y
            },
            // A
            {
                -276,    // i4X
                -326    // i4Y
            },
            // TL84
            {
                -112,    // i4X
                -309    // i4Y
            },
            // CWF
            {
                -92,    // i4X
                -376    // i4Y
            },
            // DNP
            {
                -25,    // i4X
                -369    // i4Y
            },
            // D65
            {
                139,    // i4X
                -315    // i4Y
            },
            // DF
            {
                80,    // i4X
                -484    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe
            {
                842,    // i4R
                512,    // i4G
                658    // i4B
            },
            // Horizon
            {
                512,    // i4R
                548,    // i4G
                1414    // i4B
            },
            // A
            {
                570,    // i4R
                512,    // i4G
                1150    // i4B
            },
            // TL84
            {
                688,    // i4R
                512,    // i4G
                892    // i4B
            },
            // CWF
            {
                775,    // i4R
                512,    // i4G
                945    // i4B
            },
            // DNP
            {
                837,    // i4R
                512,    // i4G
                850    // i4B
            },
            // D65
            {
                956,    // i4R
                512,    // i4G
                629    // i4B
            },
            // DF
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
            }
        },
        // Rotation matrix parameter
        {
            3,    // i4RotationAngle
            256,    // i4Cos
            13    // i4Sin
        },
        // Daylight locus parameter
        {
            -141,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -170,    // i4RightBound
            -800,    // i4LeftBound
            -235,    // i4UpperBound
            -345    // i4LowerBound
            },
            // Warm fluorescent
            {
            -170,    // i4RightBound
            -800,    // i4LeftBound
            -345,    // i4UpperBound
            -485    // i4LowerBound
            },
            // Fluorescent
            {
            -75,    // i4RightBound
            -170,    // i4LeftBound
            -235,    // i4UpperBound
            -345    // i4LowerBound
            },
            // CWF
            {
            -75,    // i4RightBound
            -170,    // i4LeftBound
            -345,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Daylight
            {
            165,    // i4RightBound
            -75,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Shade
            {
            525,    // i4RightBound
            165,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            165,    // i4RightBound
            -75,    // i4LeftBound
            -400,    // i4UpperBound
            -485    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            525,    // i4RightBound
            -800,    // i4LeftBound
            0,    // i4UpperBound
            -485    // i4LowerBound
            },
            // Daylight
            {
            190,    // i4RightBound
            -75,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Cloudy daylight
            {
            290,    // i4RightBound
            115,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Shade
            {
            390,    // i4RightBound
            115,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Twilight
            {
            -75,    // i4RightBound
            -235,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Fluorescent
            {
            189,    // i4RightBound
            -212,    // i4LeftBound
            -259,    // i4UpperBound
            -426    // i4LowerBound
            },
            // Warm fluorescent
            {
            -176,    // i4RightBound
            -376,    // i4LeftBound
            -259,    // i4UpperBound
            -426    // i4LowerBound
            },
            // Incandescent
            {
            -176,    // i4RightBound
            -376,    // i4LeftBound
            -235,    // i4UpperBound
            -400    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain
        {
            // Daylight
            {
            865,    // i4R
            512,    // i4G
            709    // i4B
            },
            // Cloudy daylight
            {
            1041,    // i4R
            512,    // i4G
            577    // i4B
            },
            // Shade
            {
            1110,    // i4R
            512,    // i4G
            538    // i4B
            },
            // Twilight
            {
            659,    // i4R
            512,    // i4G
            958    // i4B
            },
            // Fluorescent
            {
            820,    // i4R
            512,    // i4G
            807    // i4B
            },
            // Warm fluorescent
            {
            584,    // i4R
            512,    // i4G
            1175    // i4B
            },
            // Incandescent
            {
            564,    // i4R
            512,    // i4G
            1138    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color
        {
            // Tungsten
            {
            25,    // i4SliderValue
            5419    // i4OffsetThr
            },
            // Warm fluorescent
            {
            0,    // i4SliderValue
            5214    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1355    // i4OffsetThr
            },
            // Daylight WB gain
            {
            775,    // i4R
            512,    // i4G
            794    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -530,    // i4RotatedXCoordinate[0]
                -415,    // i4RotatedXCoordinate[1]
                -251,    // i4RotatedXCoordinate[2]
                -164,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T)};

    if (CameraDataType > CAMERA_DATA_AE_PLINETABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        default:
            break;
    }
    return 0;
}}; // NSFeature


