#include "MsgStreamBuffer.h"
#include "../Utils/Memory/MemoryUtility.h"
//TODO: Update for shard.
#define MSG_WRITE_BYTES_FN_OFFSET		0x00402DF0
#define MSG_READ_BYTES_FN_OFFSET		0x00403DF0
#define MSG_STREAM_CAPACITY			0x1000

WORD CMsgStreamBuffer::GetID() const
{
	void* pAddr = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x1050, void*);
	if (pAddr == NULL)
		return 0;
	return *(WORD*)(pAddr);
}

WORD CMsgStreamBuffer::GetReadPos() const
{
	return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x103C, WORD);
}

WORD CMsgStreamBuffer::GetWritePos() const
{
	return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x103E, WORD);
}

void CMsgStreamBuffer::SetMsgID(WORD wMsgID)
{
	void* pAddr = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x1050, void*);
	if (pAddr != NULL)
		*(WORD*)pAddr = wMsgID;
}

void CMsgStreamBuffer::SetReadPos(WORD wPos)
{
	MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x103C, WORD, wPos);
}

void CMsgStreamBuffer::SetWritePos(WORD wPos)
{
	MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x103E, WORD, wPos);
}

void CMsgStreamBuffer::Read(void* dest, __int16 count)
{
	if (dest == NULL || count <= 0 || !CanRead(static_cast<size_t>(count)))
		return;
	__asm pushad;
	__asm pushfd;

	//Setup arguments.
	__asm push dest;
	__asm mov esi, this;
	__asm mov di, count;

	//Call the function.
	__asm mov edx, MSG_READ_BYTES_FN_OFFSET;
	__asm call edx;

	__asm popfd;
	__asm popad;
}

std::string CMsgStreamBuffer::ReadStringA()
{
	std::string value;
	if (!TryReadStringA(value, MSG_STREAM_CAPACITY - sizeof(WORD)))
		return std::string();
	return value;
}

bool CMsgStreamBuffer::CanRead(size_t count) const
{
	const WORD readPosition = GetReadPos();
	const WORD writePosition = GetWritePos();
	return writePosition >= readPosition &&
		count <= static_cast<size_t>(writePosition - readPosition);
}

bool CMsgStreamBuffer::TryReadStringA(std::string& value, size_t maximumLength)
{
	value.clear();
	WORD length = 0;
	if (!TryRead(length) || length > maximumLength || !CanRead(length))
		return false;
	if (length == 0)
		return true;
	value.resize(length);
	Read(&value[0], static_cast<__int16>(length));
	return true;
}



//__int16 __userpurge DEC_WritePacketBytes_sub_404090@<ax>(unsigned __int16 size_2@<ax>, srCMsgStreamBuffer_data *packet@<ecx>, void *Src)
void CMsgStreamBuffer::Write(const void* src, size_t count)
{
	if (src == NULL || count == 0 || !CanWrite(count))
		return;
	__asm pushad;
	__asm pushfd;

	//const void* data
	__asm push src;

	//MsgStreamBuffer* msg
	__asm mov ecx, this;

	//Count of bytes to write, on ax but we don't really care.
	__asm mov eax, count;

	//Addr for call.
	__asm mov edx, MSG_WRITE_BYTES_FN_OFFSET;
	__asm call edx;

	__asm popfd;
	__asm popad;
}

void CMsgStreamBuffer::WriteStringA(const std::string& str)
{
	TryWriteStringA(str, MSG_STREAM_CAPACITY - sizeof(WORD));
}

bool CMsgStreamBuffer::CanWrite(size_t count) const
{
	const size_t writePosition = GetWritePos();
	return writePosition <= MSG_STREAM_CAPACITY && count <= MSG_STREAM_CAPACITY - writePosition;
}

bool CMsgStreamBuffer::TryWriteStringA(const std::string& str, size_t maximumLength)
{
	if (str.size() > maximumLength || str.size() > 0xFFFF ||
		!CanWrite(sizeof(WORD) + str.size()))
		return false;
	const WORD length = static_cast<WORD>(str.size());
	Write<WORD>(length);
	if (length > 0)
		Write(str.data(), length);
	return true;
}
