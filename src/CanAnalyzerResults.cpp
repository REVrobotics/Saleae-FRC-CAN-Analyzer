#include "CanAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "CanAnalyzer.h"
#include "CanAnalyzerSettings.h"
#include <iostream>
#include <sstream>

const char* DeviceTypeLookup[NUM_DEVICE_TYPE + 1] = {
	"Broadcast",
	"RobotCtrl",
	"motorCtrl",
	"relayCtrl",
	"gyroSensor",
	"Accelerometer",
	"Ultrasonic",
	"Geartooth",
	"PDP",
	"PCM",
	"Misc",
	"IOBreakout",
	"dev_rsvd12","dev_rsvd13","dev_rsvd14","dev_rsvd15",
	"dev_rsvd16","dev_rsvd17","dev_rsvd18","dev_rsvd19",
	"dev_rsvd20","dev_rsvd21","dev_rsvd22","dev_rsvd23",
	"dev_rsvd24","dev_rsvd25","dev_rsvd26","dev_rsvd27",
	"dev_rsvd28","dev_rsvd29","dev_rsvd30",
	"FWUpdate",
	"Invalid"
};

const char* ManufacturerLookup[NUM_MANUFACTURER + 1] = {
	"Broadcast",
	"NI",
	"TI",
	"DEKA",
	"CTRE",
	"MindSensors", //Don't know  if this is the case?
	"REV",
	"Unknown",
	"Invalid"
};

#pragma warning(disable: 4800) //warning C4800: 'U64' : forcing value to bool 'true' or 'false' (performance warning)

CanAnalyzerResults::CanAnalyzerResults( CanAnalyzer* analyzer, CanAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

CanAnalyzerResults::~CanAnalyzerResults()
{
}

void CanAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	switch( frame.mType )
	{
	case IdentifierField:
	case IdentifierFieldEx:
		{
			char number_str[128];
			bool isFRCFrame = mSettings->mIsFRC || (frame.mType == IdentifierField);

			std::stringstream ss;
			AddResultString( "Id" );


			if (mSettings->mIsFRC)
			{
				U64 CANID = (frame.mData1 & CANID_MASK) >> CANID_SHIFT;
				AnalyzerHelpers::GetNumberString(CANID, display_base, 12, number_str, 128);

				//Small format is id: CANID
				ss << "Id: " << number_str;
				AddResultString(ss.str().c_str());
				ss.str("");

				DisplayStringFromData(frame.mData1, display_base, number_str, 128);
				ss << number_str;
				AddResultString(ss.str().c_str());
				ss.str("");
			}
			else //Revert to normal CAN if IsFRC is not checked
			{
				if (frame.mType == IdentifierField)
					AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 12, number_str, 128);
				else
					AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str, 128);

				ss << "Id: " << number_str;
				AddResultString(ss.str().c_str());
				ss.str("");

				ss << "Identifier: " << number_str;
				AddResultString(ss.str().c_str());
				ss.str("");

				if (frame.mType == IdentifierField)
					ss << "Standard CAN Identifier: " << number_str;
				else
					ss << "Extended CAN Identifier: " << number_str;
			}

			if( frame.HasFlag( REMOTE_FRAME ) == true )
			{
				ss << " (RTR)";
			}

			AddResultString( ss.str().c_str() );
		}
		break;
	case ControlField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );

			std::stringstream ss;
			AddResultString( "Ctrl" );

			ss << "Ctrl: " << number_str;
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Control Field: " << number_str;
			AddResultString( ss.str().c_str() );

			ss << " bytes";
			AddResultString( ss.str().c_str() );
		}
		break;
	case DataField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

			AddResultString( number_str );

			std::stringstream ss;
			ss << "Data: " << number_str;
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Data Field Byte: " << number_str;
			AddResultString( ss.str().c_str() );
		}
		break;
	case CrcField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 15, number_str, 128 );

			AddResultString( "CRC" );

			std::stringstream ss;
			ss << "CRC: " << number_str;
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "CRC value: " << number_str;
			AddResultString( ss.str().c_str() );
		}
		break;	
	case AckField:
		{
			if( bool( frame.mData1 ) == true )
				AddResultString( "ACK" );
			else
				AddResultString( "NAK" );
		}
		break;	
	case CanError:
		{
			AddResultString( "E" );
			AddResultString( "Error" );
		}
		break;
	}
}


void CanAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "Time [s],Packet,Type,Identifier,Control,Data,CRC,ACK" << std::endl;
	U64 num_frames = GetNumFrames();
	U64 num_packets = GetNumPackets();
	for( U32 i=0; i < num_packets; i++ )
	{
		if( i != 0 )
		{
			//below, we "continue" the loop rather than run to the end.  So we need to save to the file here.
			ss << std::endl;

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );


			if( UpdateExportProgressAndCheckForCancel( i, num_packets ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
		}


		U64 first_frame_id;
		U64 last_frame_id;
		GetFramesContainedInPacket( i, &first_frame_id, &last_frame_id );
		Frame frame = GetFrame( first_frame_id );
		
		//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char packet_str[128];
		AnalyzerHelpers::GetNumberString( i, Decimal, 0, packet_str, 128 );

		if( frame.HasFlag( REMOTE_FRAME ) == false )
			ss << time_str << "," << packet_str << ",DATA";
		else
			ss << time_str << "," << packet_str << ",REMOTE";
		
		U64 frame_id = first_frame_id;

		frame = GetFrame( frame_id );

		char number_str[128];

		if( frame.mType == IdentifierField )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 12, number_str, 128);
			ss << "," << number_str;
			++frame_id;
		}
		else if( frame.mType == IdentifierFieldEx )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 32, number_str, 128 );
			ss << "," << number_str;
			++frame_id;
		}
		else
		{
			ss << ",";
		}

		if( frame_id > last_frame_id )
			continue;

		frame = GetFrame( frame_id );
		if( frame.mType == ControlField )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128);
			ss << "," << number_str;
			++frame_id;
		}
		else
		{
			ss << ",";
		}
		ss << ",";
		if( frame_id > last_frame_id )
			continue;

		for( ; ; )
		{
			frame = GetFrame( frame_id );
			if( frame.mType != DataField )
				break;

			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			ss << number_str;
			if( frame_id == last_frame_id )
				break;

			++frame_id;
			if( GetFrame( frame_id ).mType == DataField )
				ss << " ";
		}

		if( frame_id > last_frame_id )
			continue;

		frame = GetFrame( frame_id );
		if( frame.mType == CrcField )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 15, number_str, 128);
			ss << "," << number_str;
			++frame_id;
		}else
		{
			ss << ",";
		}
		if( frame_id > last_frame_id )
			continue;

		frame = GetFrame( frame_id );
		if( frame.mType == AckField )
		{
			if( bool( frame.mData1 ) == true )
				ss << "," << "ACK";
			else
				ss << "," << "NAK";

			++frame_id;
		}else
		{
			ss << ",";
		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void CanAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();

	Frame frame = GetFrame( frame_index );

	switch( frame.mType )
	{
	case IdentifierField:
	case IdentifierFieldEx:
		{
			bool isFRCFrame = mSettings->mIsFRC || (frame.mType == IdentifierField);
			char number_str[128];

			if (isFRCFrame)
			{
				std::stringstream ss;

				GetDeviceTypeString(frame.mData1, number_str, 128);
				ss << "Device Type: " << number_str;
				AddTabularText(ss.str().c_str());

				GetManufacturerString(frame.mData1, number_str, 128);
				ss.str(""); ss << "Manufacturer: " << number_str;
				AddTabularText(ss.str().c_str());

				GetAPIClassString(frame.mData1,display_base, number_str, 128);
				ss.str(""); ss << "API Class: " << number_str;
				AddTabularText(ss.str().c_str());

				GetAPIIndexString(frame.mData1, display_base, number_str, 128);
				ss.str(""); ss << "API Index: " << number_str;
				AddTabularText(ss.str().c_str());

				GetCANIDString(frame.mData1, display_base, number_str, 128);
				ss.str(""); ss << "CANID: " << number_str;
				AddTabularText(ss.str().c_str());
				
			}else
			{
				if (frame.mType == IdentifierField)
					AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 12, number_str, 128);
				else
					AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str, 128);

				std::stringstream ss;


				if (frame.HasFlag(REMOTE_FRAME) == false)
				{
					if (frame.mType == IdentifierField)
						ss << "Standard CAN Identifier: " << number_str;
					else
						ss << "Extended CAN Identifier: " << number_str;
				}
				else
				{
					if (frame.mType == IdentifierField)
						ss << "Standard CAN Identifier: " << number_str << " (RTR)";
					else
						ss << "Extended CAN Identifier: " << number_str << " (RTR)";
				}

				AddTabularText(ss.str().c_str());
			}
		}
		break;
	case ControlField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );

			std::stringstream ss;
			
			ss << "Control Field: " << number_str;
			ss << " bytes";
			AddTabularText( ss.str().c_str() );

		}
		break;
	case DataField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

			std::stringstream ss;
			
			ss << "Data Field Byte: " << number_str;
			AddTabularText( ss.str().c_str() );
		}
		break;
	case CrcField:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 15, number_str, 128 );
						
			std::stringstream ss;
			
			ss << "CRC value: " << number_str;
			AddTabularText( ss.str().c_str() );
		}
		break;	
	case AckField:
		{
			if( bool( frame.mData1 ) == true )
				AddTabularText( "ACK" );
			else
				AddTabularText( "NAK" );
		}
		break;	
	case CanError:
		{
			AddTabularText( "Error" );
		}
		break;
	}
}

void CanAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void CanAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void CanAnalyzerResults::GetDeviceTypeString(U64 frame, char* output, U32 result_string_max_length)
{
	U64 DeviceType = (frame & DEVICE_TYPE_MASK) >> DEVICE_TYPE_SHIFT;
	if (DeviceType > NUM_DEVICE_TYPE)
		DeviceType = NUM_DEVICE_TYPE;
	strncpy(output, DeviceTypeLookup[DeviceType], result_string_max_length-1);
	output[result_string_max_length - 1] = '\0';
}

void CanAnalyzerResults::GetManufacturerString(U64 frame, char* output, U32 result_string_max_length)
{
	U64 Manufacturer = (frame & MANUFACTURER_MASK) >> MANUFACTURER_SHIFT;
	if (Manufacturer > NUM_MANUFACTURER)
		Manufacturer = NUM_MANUFACTURER;
	strncpy(output, ManufacturerLookup[Manufacturer], result_string_max_length - 1);
	output[result_string_max_length - 1] = '\0';
}

void CanAnalyzerResults::GetAPIClassString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length)
{
	U64 APIClass = (frame & API_CLASS_MASK) >> API_CLASS_SHIFT;
	AnalyzerHelpers::GetNumberString(APIClass, display_base, API_CLASS_BITS, output, result_string_max_length);
}

void CanAnalyzerResults::GetAPIIndexString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length)
{
	U64 APIIndex = (frame & API_INDEX_MASK) >> API_INDEX_SHIFT;
	AnalyzerHelpers::GetNumberString(APIIndex, display_base, API_INDEX_BITS, output, result_string_max_length);
}

void CanAnalyzerResults::GetCANIDString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length)
{
	U64 CANID = (frame & CANID_MASK) >> CANID_SHIFT;
	AnalyzerHelpers::GetNumberString(CANID, display_base, CANID_BITS, output, result_string_max_length);
}

void CanAnalyzerResults::DisplayStringFromData(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length)
{
	U64 DeviceType = (frame & DEVICE_TYPE_MASK) >> DEVICE_TYPE_SHIFT;
	U64 Manufacturer = (frame & MANUFACTURER_MASK) >> MANUFACTURER_SHIFT;
	U64 APIClass = (frame & API_CLASS_MASK) >> API_CLASS_SHIFT;
	U64 APIIndex = (frame & API_INDEX_MASK) >> API_INDEX_SHIFT;
	U64 CANID = (frame & CANID_MASK) >> CANID_SHIFT;

	std::stringstream ss;

	if (DeviceType > NUM_DEVICE_TYPE)
		DeviceType = NUM_DEVICE_TYPE;
	if (Manufacturer > NUM_MANUFACTURER)
		Manufacturer = NUM_MANUFACTURER;

	ss.str("");
	ss << "Dev: " << DeviceTypeLookup[DeviceType] << " M: " << ManufacturerLookup[Manufacturer];

	U32 numChars = 128;

	char tmp[128];
	AnalyzerHelpers::GetNumberString(APIClass, display_base, API_CLASS_BITS, tmp, numChars);
	ss << " Class: " << tmp;
	AnalyzerHelpers::GetNumberString(APIIndex, display_base, API_INDEX_BITS, tmp, numChars);
	ss << " Idx: " << tmp;
	AnalyzerHelpers::GetNumberString(CANID, display_base, CANID_BITS, tmp, numChars);
	ss << " ID: " << tmp;

	strncpy(output, ss.str().c_str(), result_string_max_length-1);
	output[result_string_max_length - 1] = '\0';
}

/*
void CanAnalyzerResults::ParseStringFromData(U64 frame,
	DisplayBase display_base,
	char* deviceType,
	char* manufacturer,
	char* apiClass,
	char* apiIndex,
	char* deviceNumber,
	U32 result_string_max_length)
{

}
*/
