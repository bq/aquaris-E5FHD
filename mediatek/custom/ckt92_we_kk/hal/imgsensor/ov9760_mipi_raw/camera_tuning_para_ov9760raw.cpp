#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov9760raw.h"
#include "camera_info_ov9760raw.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"


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
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
        75925,    // i4R_AVG
        19233,    // i4R_STD
        109200,    // i4B_AVG
        26036,    // i4B_STD
        {  // i4P00[9]
            3680000, -825000, -280000, -167500, 2787500, -60000, 30000, -1527500, 4057500
        },
        {  // i4P10[9]
            420143, -554970, 137078, 20433, -99575, 79142, 122107, 416401, -542748
        },
        {  // i4P01[9]
            -274908, 136333, 149763, 85175, -445723, 360547, 34499, 221560, -256687
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
            1136,    // u4MinGain, 1024 base = 1x
            16384,    // u4MaxGain, 16x
            140,    // u4MiniISOGain, ISOxx  
            32,    // u4GainStepUnit, 1x/8 
            28934,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            28934,    // u4VideoExpUnit  
            30,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            28934,    // u4CapExpUnit 
            30,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            24,    // u4LensFno, Fno = 2.8
            235    // zhangjiano modify u4FocusLength_100x  
        },
        // rHistConfig
        {
            2,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {75, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            TRUE,    // bEnableCaptureThres
            TRUE,    // bEnableVideoThres
            TRUE,    // bEnableStrobeThres
            42,    // u4AETarget
            0,    // u4StrobeAETarget
            45,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            0,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM] 
            90,    // u4InDoorEV = 9.0, 10 base 
            -10,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            5,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            64,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            8,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            8,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            58    // u4FlatnessStrength
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
                889,    // i4R
                512,    // i4G
                744    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                0,    // i4X
                0    // i4Y
            },
            // Horizon
            {
                -460,    // i4X
                -277    // i4Y
            },
            // A
            {
                -365,    // i4X
                -326    // i4Y
            },
            // TL84
            {
                -206,    // i4X
                -416    // i4Y
            },
            // CWF
            {
                -153,    // i4X
                -516    // i4Y
            },
            // DNP
            {
                -19,    // i4X
                -418    // i4Y
            },
            // D65
            {
                66,    // i4X
                -342    // i4Y
            },
            // DF
            {
                30,    // i4X
                -440    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                0,    // i4X
                0    // i4Y
            },
            // Horizon
            {
                -460,    // i4X
                -277    // i4Y
            },
            // A
            {
                -365,    // i4X
                -326    // i4Y
            },
            // TL84
            {
                -206,    // i4X
                -416    // i4Y
            },
            // CWF
            {
                -153,    // i4X
                -516    // i4Y
            },
            // DNP
            {
                -19,    // i4X
                -418    // i4Y
            },
            // D65
            {
                66,    // i4X
                -342    // i4Y
            },
            // DF
            {
                30,    // i4X
                -440    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                656,    // i4G
                1778    // i4B
            },
            // A 
            {
                512,    // i4R
                539,    // i4G
                1374    // i4B
            },
            // TL84 
            {
                681,    // i4R
                512,    // i4G
                1188    // i4B
            },
            // CWF 
            {
                837,    // i4R
                512,    // i4G
                1266    // i4B
            },
            // DNP 
            {
                879,    // i4R
                512,    // i4G
                925    // i4B
            },
            // D65 
            {
                889,    // i4R
                512,    // i4G
                744    // i4B
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
            0,    // i4RotationAngle
            256,    // i4Cos
            0    // i4Sin
        },
        // Daylight locus parameter
        {
            -115,    // i4SlopeNumerator
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
            -256,    // i4RightBound
            -906,    // i4LeftBound
            -251,    // i4UpperBound
            -351    // i4LowerBound
            },
            // Warm fluorescent
            {
            -256,    // i4RightBound
            -906,    // i4LeftBound
            -351,    // i4UpperBound
            -471    // i4LowerBound
            },
            // Fluorescent
            {
            -69,    // i4RightBound
            -256,    // i4LeftBound
            -256,    // i4UpperBound
            -466    // i4LowerBound
            },
            // CWF
            {
            -69,    // i4RightBound
            -256,    // i4LeftBound
            -466,    // i4UpperBound
            -566    // i4LowerBound
            },
            // Daylight
            {
            91,    // i4RightBound
            -69,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Shade
            {
            451,    // i4RightBound
            91,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            103,    // i4RightBound
            -54,    // i4LeftBound
            -444,    // i4UpperBound
            -574    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            451,    // i4RightBound
            -906,    // i4LeftBound
            -226,    // i4UpperBound
            -566    // i4LowerBound
            },
            // Daylight
            {
            116,    // i4RightBound
            -69,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Cloudy daylight
            {
            216,    // i4RightBound
            41,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Shade
            {
            316,    // i4RightBound
            41,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Twilight
            {
            -69,    // i4RightBound
            -229,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
            },
            // Fluorescent
            {
            116,    // i4RightBound
            -306,    // i4LeftBound
            -292,    // i4UpperBound
            -566    // i4LowerBound
            },
            // Warm fluorescent
            {
            -265,    // i4RightBound
            -465,    // i4LeftBound
            -292,    // i4UpperBound
            -566    // i4LowerBound
            },
            // Incandescent
            {
            -265,    // i4RightBound
            -465,    // i4LeftBound
            -262,    // i4UpperBound
            -443    // i4LowerBound
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
            852,    // i4R
            512,    // i4G
            799    // i4B
            },
            // Cloudy daylight
            {
            982,    // i4R
            512,    // i4G
            693    // i4B
            },
            // Shade
            {
            1051,    // i4R
            512,    // i4G
            648    // i4B
            },
            // Twilight
            {
            674,    // i4R
            512,    // i4G
            1010    // i4B
            },
            // Fluorescent
            {
            805,    // i4R
            512,    // i4G
            1041    // i4B
            },
            // Warm fluorescent
            {
            558,    // i4R
            512,    // i4G
            1500    // i4B
            },
            // Incandescent
            {
            503,    // i4R
            512,    // i4G
            1352    // i4B
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
            0,    // i4SliderValue
            6867    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            5390    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1341    // i4OffsetThr
            },
            // Daylight WB gain
            {
            813,    // i4R
            512,    // i4G
            525    // i4B
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
                -526,    // i4RotatedXCoordinate[0]
                -431,    // i4RotatedXCoordinate[1]
                -272,    // i4RotatedXCoordinate[2]
                -85,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace

const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    #include INCLUDE_FILENAME_TSF_PARA
    #include INCLUDE_FILENAME_TSF_DATA
};

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
                                             sizeof(AE_PLINETABLE_T),
                                             0,
                                             sizeof(CAMERA_TSF_TBL_STRUCT)};

    if (CameraDataType > CAMERA_DATA_TSF_TABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
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
        case CAMERA_DATA_TSF_TABLE:
            memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
            break;
        default:
            break;
    }
    return 0;
}}; // NSFeature


