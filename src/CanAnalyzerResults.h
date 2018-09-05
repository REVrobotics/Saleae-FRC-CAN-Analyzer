#ifndef CAN_ANALYZER_RESULTS
#define CAN_ANALYZER_RESULTS

#include <AnalyzerResults.h>

enum CanFrameType { IdentifierField, IdentifierFieldEx, ControlField, DataField, CrcField, AckField, CanError };
#define REMOTE_FRAME ( 1 << 0 )

//#define FRAMING_ERROR_FLAG ( 1 << 0 )
//#define PARITY_ERROR_FLAG ( 1 << 1 )

#define DEVICE_TYPE_MASK	0x1F000000
#define MANUFACTURER_MASK	0x00FF0000
#define API_CLASS_MASK		0x0000FC00
#define API_INDEX_MASK		0x000003C0
#define CANID_MASK			0x0000003F

#define DEVICE_TYPE_SHIFT	0x18
#define MANUFACTURER_SHIFT	0x10
#define API_CLASS_SHIFT		0xA
#define API_INDEX_SHIFT		0x6
#define CANID_SHIFT			0x0

#define API_CLASS_BITS		6
#define API_INDEX_BITS		4
#define CANID_BITS			6

#define NUM_DEVICE_TYPE 32
#define NUM_MANUFACTURER 8

class CanAnalyzer;
class CanAnalyzerSettings;

class CanAnalyzerResults : public AnalyzerResults
{
public:
	CanAnalyzerResults( CanAnalyzer* analyzer, CanAnalyzerSettings* settings );
	virtual ~CanAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	void DisplayStringFromData(U64 frame, DisplayBase display_base, char* str, U32 result_string_max_length);
	void GetDeviceTypeString(U64 frame, char* output, U32 result_string_max_length);
	void GetManufacturerString(U64 frame, char* output, U32 result_string_max_length);
	void GetAPIClassString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length);
	void GetAPIIndexString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length);
	void GetCANIDString(U64 frame, DisplayBase display_base, char* output, U32 result_string_max_length);

protected:  //vars
	CanAnalyzerSettings* mSettings;
	CanAnalyzer* mAnalyzer;
};

#endif //CAN_ANALYZER_RESULTS
