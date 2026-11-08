// Android cikis katmani: OpenSL ES buffer-queue, 44.1 kHz stereo int16.
#ifdef __ANDROID__
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <string.h>

#include "engine.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FS12Audio", __VA_ARGS__)

namespace fs12 {
namespace audio {

namespace {

const int kFramesPerBuffer = 1024;
const int kBufferCount = 3;

class OpenSlOutput : public Output {
public:
    ~OpenSlOutput() override { stop(); }

    bool start(Engine* engine) override {
        if (engine_) return true;
        engine_ = engine;
        memset(buffers_, 0, sizeof(buffers_));

        if (slCreateEngine(&slObj_, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return fail("slCreateEngine");
        if ((*slObj_)->Realize(slObj_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return fail("engine Realize");
        SLEngineItf eng;
        if ((*slObj_)->GetInterface(slObj_, SL_IID_ENGINE, &eng) != SL_RESULT_SUCCESS) return fail("engine itf");
        if ((*eng)->CreateOutputMix(eng, &mixObj_, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return fail("mix");
        if ((*mixObj_)->Realize(mixObj_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return fail("mix Realize");

        SLDataLocator_AndroidSimpleBufferQueue q = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, (SLuint32)kBufferCount};
        SLDataFormat_PCM fmt = {SL_DATAFORMAT_PCM,
                                2,
                                SL_SAMPLINGRATE_44_1,
                                SL_PCMSAMPLEFORMAT_FIXED_16,
                                SL_PCMSAMPLEFORMAT_FIXED_16,
                                SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                SL_BYTEORDER_LITTLEENDIAN};
        SLDataSource src = {&q, &fmt};
        SLDataLocator_OutputMix om = {SL_DATALOCATOR_OUTPUTMIX, mixObj_};
        SLDataSink snk = {&om, nullptr};
        const SLInterfaceID ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
        const SLboolean req[1] = {SL_BOOLEAN_TRUE};
        if ((*eng)->CreateAudioPlayer(eng, &playerObj_, &src, &snk, 1, ids, req) != SL_RESULT_SUCCESS) return fail("player");
        if ((*playerObj_)->Realize(playerObj_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return fail("player Realize");
        if ((*playerObj_)->GetInterface(playerObj_, SL_IID_PLAY, &play_) != SL_RESULT_SUCCESS) return fail("play itf");
        if ((*playerObj_)->GetInterface(playerObj_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queue_) != SL_RESULT_SUCCESS)
            return fail("queue itf");
        if ((*queue_)->RegisterCallback(queue_, &OpenSlOutput::onBuffer, this) != SL_RESULT_SUCCESS) return fail("callback");

        for (int i = 0; i < kBufferCount; ++i) {
            engine_->render(buffers_[i], kFramesPerBuffer);
            (*queue_)->Enqueue(queue_, buffers_[i], sizeof(buffers_[i]));
        }
        next_ = 0;
        if ((*play_)->SetPlayState(play_, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) return fail("play state");
        return true;
    }

    void stop() override {
        if (play_) (*play_)->SetPlayState(play_, SL_PLAYSTATE_STOPPED);
        if (queue_) (*queue_)->Clear(queue_);
        if (playerObj_) (*playerObj_)->Destroy(playerObj_);
        if (mixObj_) (*mixObj_)->Destroy(mixObj_);
        if (slObj_) (*slObj_)->Destroy(slObj_);
        playerObj_ = mixObj_ = slObj_ = nullptr;
        play_ = nullptr;
        queue_ = nullptr;
        engine_ = nullptr;
    }

    void setPaused(bool paused) override {
        if (play_) (*play_)->SetPlayState(play_, paused ? SL_PLAYSTATE_PAUSED : SL_PLAYSTATE_PLAYING);
    }

private:
    bool fail(const char* what) {
        LOGE("OpenSL ES hatasi: %s", what);
        stop();
        return false;
    }

    static void onBuffer(SLAndroidSimpleBufferQueueItf, void* ctx) {
        OpenSlOutput* self = static_cast<OpenSlOutput*>(ctx);
        if (!self->engine_) return;
        int16_t* buf = self->buffers_[self->next_];
        self->engine_->render(buf, kFramesPerBuffer);
        (*self->queue_)->Enqueue(self->queue_, buf, sizeof(self->buffers_[0]));
        self->next_ = (self->next_ + 1) % kBufferCount;
    }

    Engine* engine_ = nullptr;
    SLObjectItf slObj_ = nullptr, mixObj_ = nullptr, playerObj_ = nullptr;
    SLPlayItf play_ = nullptr;
    SLAndroidSimpleBufferQueueItf queue_ = nullptr;
    int16_t buffers_[kBufferCount][kFramesPerBuffer * 2];
    int next_ = 0;
};

}  // namespace

Output* createDefaultOutput() { return new OpenSlOutput(); }

}  // namespace audio
}  // namespace fs12
#endif  // __ANDROID__
