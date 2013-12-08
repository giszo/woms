#ifndef CODEC_BASECODEC_H_INCLUDED
#define CODEC_BASECODEC_H_INCLUDED

#include <string>
#include <stdexcept>
#include <memory>

namespace codec
{

class CodecException : public std::runtime_error
{
    public:
	CodecException(const std::string& error) :
	    runtime_error(error)
	{}
};

struct MediaInfo
{
    /// sampling rate
    int m_rate;
    /// number of channels
    int m_channels;
    /// the number of samples in the resource
    size_t m_samples;
};

class BaseCodec
{
    public:
	virtual ~BaseCodec()
	{}

	virtual void open(const std::string& file) = 0;

	/// returns the sampling rate of the media stream
	virtual int getRate() = 0;
	/// returns the number of channels in the media stream
	virtual int getChannels() = 0;

	/// returns informations about the media
	virtual MediaInfo getMediaInfo() = 0;

	/**
	 * Decodes the next part of the media stream.
	 * @return false is returned at the end of the stream
	 */
	virtual bool decode(int16_t*& samples, size_t& count) = 0;

	static std::shared_ptr<BaseCodec> openFile(const std::string& file);
};

}

#endif
