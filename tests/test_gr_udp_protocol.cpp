//
// Created by RGAA on 12/08/2026.
// Unit tests for gr_udp_protocol.h (shard / reassemble / ctrl packets)
//

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>

#include "tc_common_new/gr_udp_protocol.h"

using namespace tc;

static std::string MakeFrameBytes(size_t size) {
    std::string s(size, '\0');
    for (size_t i = 0; i < size; i++) s[i] = (char)(i % 251);
    return s;
}

static GrUdpProtocol::VideoFrameMeta MakeMeta(uint32_t frame_index, bool key) {
    GrUdpProtocol::VideoFrameMeta meta;
    meta.frame_index_ = frame_index;
    meta.timestamp_ms_ = 123456;
    meta.key_ = key;
    meta.codec_ = GrUdpProtocol::kCodecH264;
    meta.frame_width_ = 1920;
    meta.frame_height_ = 1080;
    meta.mon_slot_ = 1;
    meta.mon_name_ = R"(\\.\DISPLAY1)";
    return meta;
}

TEST(GrUdpProtocol, CommonHeader) {
    char buf[4];
    GrUdpProtocol::WriteCommon(buf, GrUdpProtocol::kPktVideo);
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 4), GrUdpProtocol::kPktVideo);
    buf[0] = 0; // break magic
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 4), 0);
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 2), 0);
}

TEST(GrUdpProtocol, ShardSmallFrameSinglePacket) {
    auto frame = MakeFrameBytes(500);
    auto meta = MakeMeta(7, true);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_EQ(pkts.size(), 1u);

    GrUdpProtocol::VideoShardInfo shard;
    ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[0]->CStr(), pkts[0]->Size(), shard));
    EXPECT_EQ(shard.frame_index_, 7u);
    EXPECT_EQ(shard.data_shards_, 1u);
    EXPECT_EQ(shard.shard_index_, 0u);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagSof);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagEof);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagKey);
    EXPECT_EQ(shard.mon_name_, meta.mon_name_);
    EXPECT_EQ(shard.frame_width_, 1920);
    EXPECT_EQ(shard.frame_size_, 500u);
    EXPECT_EQ(shard.codec_, GrUdpProtocol::kCodecH264);
    ASSERT_EQ(shard.payload_len_, 500u);
    EXPECT_EQ(std::memcmp(shard.payload_, frame.data(), 500), 0);
}

TEST(GrUdpProtocol, ShardLargeFrameMultiplePackets) {
    auto frame = MakeFrameBytes(10000);
    auto meta = MakeMeta(9, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);
    for (size_t i = 0; i < pkts.size(); i++) {
        EXPECT_LE(pkts[i]->Size(), GrUdpProtocol::kDefaultMtu);
        GrUdpProtocol::VideoShardInfo shard;
        ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[i]->CStr(), pkts[i]->Size(), shard));
        EXPECT_EQ(shard.data_shards_, pkts.size());
        EXPECT_EQ(shard.shard_index_, i);
        EXPECT_EQ(shard.flags_ & GrUdpProtocol::kFlagSof, i == 0 ? GrUdpProtocol::kFlagSof : 0);
        // mon_name only on SOF
        if (i == 0) EXPECT_EQ(shard.mon_name_, meta.mon_name_);
        else EXPECT_TRUE(shard.mon_name_.empty());
        // frame_size only on SOF
        if (i == 0) EXPECT_EQ(shard.frame_size_, 10000u);
        else EXPECT_EQ(shard.frame_size_, 0u);
    }
}

TEST(GrUdpProtocol, ReassembleInOrder) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(3, true);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };

    for (auto& p : pkts) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].frame_index_, 3u);
    EXPECT_TRUE(frames[0].key_);
    EXPECT_EQ(frames[0].mon_name_, meta.mon_name_);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, ReassembleOutOfOrder) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(4, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 2u);

    // P frame as the very first frame of the stream is decodable (no prior loss)
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };

    // deliver: SOF first, then reverse the rest
    reasm.AddPacket(pkts[0]->CStr(), pkts[0]->Size());
    for (size_t i = pkts.size() - 1; i >= 1; i--) reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    ASSERT_EQ(frames.size(), 1u);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, LossDeclaresAndDropsPUntilKey) {
    auto frame_n = MakeFrameBytes(8000);
    auto meta_n = MakeMeta(10, false);
    auto pkts_n = GrUdpProtocol::ShardVideoFrame(meta_n, frame_n.data(), frame_n.size());
    ASSERT_GT(pkts_n.size(), 1u);

    auto frame_n1 = MakeFrameBytes(600);
    auto meta_n1 = MakeMeta(11, false);
    auto pkts_n1 = GrUdpProtocol::ShardVideoFrame(meta_n1, frame_n1.data(), frame_n1.size());

    auto frame_key = MakeFrameBytes(700);
    auto meta_key = MakeMeta(12, true);
    auto pkts_key = GrUdpProtocol::ShardVideoFrame(meta_key, frame_key.data(), frame_key.size());

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    std::vector<uint32_t> lost;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };

    // frame 10: deliver all but the last shard -> stuck incomplete
    for (size_t i = 0; i + 1 < pkts_n.size(); i++) reasm.AddPacket(pkts_n[i]->CStr(), pkts_n[i]->Size());
    EXPECT_TRUE(frames.empty());

    // frame 11 (newer) arrives -> frame 10 declared lost
    for (auto& p : pkts_n1) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 10u);
    // frame 11 is a P frame after a loss -> completed but dropped
    EXPECT_TRUE(frames.empty());

    // key frame 12 -> delivered, stream recovered
    for (auto& p : pkts_key) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(frames[0].key_);
    EXPECT_EQ(frames[0].frame_index_, 12u);
}

TEST(GrUdpProtocol, JoinMidFrameDeclaresLoss) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(20, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);

    GrUdpFrameReassembler reasm;
    std::vector<uint32_t> lost;
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };
    reasm.on_frame_ = [](const GrUdpFrameReassembler::CompleteFrame&) { FAIL() << "should not complete"; };

    // first seen shard is NOT a SOF -> mid-frame join
    reasm.AddPacket(pkts[1]->CStr(), pkts[1]->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 20u);
}

TEST(GrUdpProtocol, CtrlRoundtrip) {
    std::string s1, s2;

    auto hello = GrUdpProtocol::BuildHello("dev-123", "stream-abc");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hello->CStr(), hello->Size(), s1, s2), GrUdpProtocol::kCtrlHello);
    EXPECT_EQ(s1, "dev-123");
    EXPECT_EQ(s2, "stream-abc");

    auto hb = GrUdpProtocol::BuildHeartbeat("stream-abc");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hb->CStr(), hb->Size(), s1, s2), GrUdpProtocol::kCtrlHeartbeat);
    EXPECT_EQ(s1, "stream-abc");

    auto idr = GrUdpProtocol::BuildIdrRequest(R"(\\.\DISPLAY2)");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(idr->CStr(), idr->Size(), s1, s2), GrUdpProtocol::kCtrlIdrRequest);
    EXPECT_EQ(s1, R"(\\.\DISPLAY2)");

    auto kick = GrUdpProtocol::BuildKick("taken over");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(kick->CStr(), kick->Size(), s1, s2), GrUdpProtocol::kCtrlKick);
    EXPECT_EQ(s1, "taken over");

    // truncated packet rejected
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hello->CStr(), hello->Size() - 3, s1, s2), 0);
}

// ---------------- FEC (Reed-Solomon, P2) ----------------

// 带 FEC 切帧:返回全部包(数据包在前、parity 包随后)
static std::vector<std::shared_ptr<Data>> ShardFec(const GrUdpProtocol::VideoFrameMeta& meta,
                                                   const std::string& frame, int fec_percent) {
    return GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size(),
                                          GrUdpProtocol::kDefaultMtu, fec_percent);
}

static uint16_t DataShardsOf(const std::vector<std::shared_ptr<Data>>& pkts) {
    GrUdpProtocol::VideoShardInfo shard;
    EXPECT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[0]->CStr(), pkts[0]->Size(), shard));
    return shard.data_shards_;
}

TEST(GrUdpProtocol, FecShardLayout) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(30, true);
    auto pkts = ShardFec(meta, frame, 20);
    uint16_t d = DataShardsOf(pkts);
    ASSERT_GT(d, 1u);
    uint16_t parity = (uint16_t)(pkts.size() - d);
    EXPECT_EQ(parity, (d * 20 + 99) / 100); // ceil(D*20%)
    for (size_t i = 0; i < pkts.size(); i++) {
        GrUdpProtocol::VideoShardInfo shard;
        ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[i]->CStr(), pkts[i]->Size(), shard));
        EXPECT_EQ(shard.data_shards_, d);
        EXPECT_EQ(shard.parity_shards_, parity);
        EXPECT_EQ(shard.fec_block_, 0);
        if (i < d) {
            EXPECT_FALSE(shard.flags_ & GrUdpProtocol::kFlagParity);
            EXPECT_EQ(shard.shard_index_, i);
            EXPECT_LE(pkts[i]->Size(), GrUdpProtocol::kDefaultMtu);
        }
        else {
            // parity 包:kFlagParity,shard_index = D+j,整包正好 mtu,无 SOF 扩展
            EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagParity);
            EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagKey); // key 帧的 parity 也带 key 标记
            EXPECT_EQ(shard.shard_index_, d + (i - d));
            EXPECT_EQ(pkts[i]->Size(), (size_t)GrUdpProtocol::kDefaultMtu);
            EXPECT_FALSE(shard.flags_ & GrUdpProtocol::kFlagSof);
        }
    }
}

TEST(GrUdpProtocol, FecRecoversOneDataShard) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(31, true);
    auto pkts = ShardFec(meta, frame, 20);
    uint16_t d = DataShardsOf(pkts);
    ASSERT_GT(pkts.size(), (size_t)d);

    // 丢一个中间数据 shard
    const size_t dropped = d / 2;
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };
    for (size_t i = 0; i < pkts.size(); i++) {
        if (i == dropped) continue;
        reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    }
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].frame_index_, 31u);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, FecRecoversShardZero) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(32, true);
    auto pkts = ShardFec(meta, frame, 20);
    uint16_t d = DataShardsOf(pkts);

    // 丢 shard 0(SOF):mon_name/分辨率/frame_size 都要从恢复块里取
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };
    for (size_t i = 1; i < pkts.size(); i++) {
        reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    }
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].mon_name_, meta.mon_name_);
    EXPECT_EQ(frames[0].frame_width_, 1920);
    EXPECT_EQ(frames[0].frame_height_, 1080);
    EXPECT_TRUE(frames[0].key_);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, FecRecoversUpToParityCount) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(33, false);
    auto pkts = ShardFec(meta, frame, 20);
    uint16_t d = DataShardsOf(pkts);
    size_t parity = pkts.size() - d;
    ASSERT_GE(parity, 2u);

    // 丢 parity_count 个数据 shard -> 仍可恢复
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };
    for (size_t i = parity; i < pkts.size(); i++) { // 跳过前 parity 个数据 shard
        reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    }
    ASSERT_EQ(frames.size(), 1u);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, FecLossBeyondParityDeclaresLoss) {
    auto frame_n = MakeFrameBytes(12000);
    auto meta_n = MakeMeta(40, false);
    auto pkts_n = ShardFec(meta_n, frame_n, 20);
    uint16_t d = DataShardsOf(pkts_n);
    size_t parity = pkts_n.size() - d;

    auto frame_key = MakeFrameBytes(700);
    auto meta_key = MakeMeta(41, true);
    auto pkts_key = ShardFec(meta_key, frame_key, 20);

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    std::vector<uint32_t> lost;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };

    // 丢 parity+1 个数据 shard -> 恢复不了,帧卡住
    for (size_t i = parity + 1; i < pkts_n.size(); i++) {
        reasm.AddPacket(pkts_n[i]->CStr(), pkts_n[i]->Size());
    }
    EXPECT_TRUE(frames.empty());
    EXPECT_TRUE(lost.empty());

    // 新帧到达 -> 旧帧判丢;key 帧正常交付,流恢复
    for (auto& p : pkts_key) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 40u);
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(frames[0].key_);
    EXPECT_EQ(frames[0].frame_index_, 41u);
}

TEST(GrUdpProtocol, FecDisabledBehavesLikeBefore) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(50, true);
    auto pkts = ShardFec(meta, frame, 0);
    uint16_t d = DataShardsOf(pkts);
    // 无 parity 包,所有包 parity_shards = 0
    EXPECT_EQ(pkts.size(), (size_t)d);
    for (auto& p : pkts) {
        GrUdpProtocol::VideoShardInfo shard;
        ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(p->CStr(), p->Size(), shard));
        EXPECT_EQ(shard.parity_shards_, 0);
        EXPECT_FALSE(shard.flags_ & GrUdpProtocol::kFlagParity);
    }

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };
    for (auto& p : pkts) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

// FEC_VALIDATION 风格:遍历每个 shard 位置单独丢弃,都能恢复出原帧
TEST(GrUdpProtocol, FecRecoverEverySingleShardPosition) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(60, true);
    auto pkts = ShardFec(meta, frame, 20);
    uint16_t d = DataShardsOf(pkts);
    ASSERT_GT(d, 1u);

    for (size_t dropped = 0; dropped < (size_t)d; dropped++) {
        GrUdpFrameReassembler reasm;
        std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
        reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
        reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) {
            FAIL() << "unexpected loss, dropped shard " << dropped << ", frame " << idx;
        };
        // 乱序投递:先 parity 再数据(跳过被丢的),覆盖 parity 先到的路径
        for (size_t i = d; i < pkts.size(); i++) reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
        for (size_t i = 0; i < (size_t)d; i++) {
            if (i == dropped) continue;
            reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
        }
        ASSERT_EQ(frames.size(), 1u) << "dropped shard " << dropped;
        ASSERT_EQ(frames[0].data_->Size(), frame.size()) << "dropped shard " << dropped;
        EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0)
            << "dropped shard " << dropped;
        EXPECT_EQ(frames[0].mon_name_, meta.mon_name_) << "dropped shard " << dropped;
    }
}

// ---------------- FRAME_STATUS 反馈 ----------------

TEST(GrUdpProtocol, FrameStatusRoundtrip) {
    auto pkt = GrUdpProtocol::BuildFrameStatus(12345, 8, 2);
    uint32_t frame_index = 0;
    uint16_t received = 0, lost = 0;
    ASSERT_TRUE(GrUdpProtocol::ParseFrameStatus(pkt->CStr(), pkt->Size(), frame_index, received, lost));
    EXPECT_EQ(frame_index, 12345u);
    EXPECT_EQ(received, 8u);
    EXPECT_EQ(lost, 2u);

    // 其它 ctrl 包/截断包不应被解析成 FrameStatus
    auto hb = GrUdpProtocol::BuildHeartbeat("stream-abc");
    ASSERT_FALSE(GrUdpProtocol::ParseFrameStatus(hb->CStr(), hb->Size(), frame_index, received, lost));
    ASSERT_FALSE(GrUdpProtocol::ParseFrameStatus(pkt->CStr(), pkt->Size() - 1, frame_index, received, lost));
    // FrameStatus 不走 ParseCtrl 字符串路径
    std::string s1, s2;
    EXPECT_EQ(GrUdpProtocol::ParseCtrl(pkt->CStr(), pkt->Size(), s1, s2), 0);
}

TEST(GrUdpProtocol, FrameStatusOnCleanComplete) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(70, true);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    const uint16_t d = DataShardsOf(pkts);

    GrUdpFrameReassembler reasm;
    struct Status { uint32_t frame; uint16_t received; uint16_t lost; };
    std::vector<Status> statuses;
    reasm.on_frame_status_ = [&](uint8_t, uint32_t f, uint16_t r, uint16_t l) {
        statuses.push_back({f, r, l});
    };
    for (auto& p : pkts) reasm.AddPacket(p->CStr(), p->Size());
    // 干净完成:received = 全部数据 shard,lost = 0,恰好一次
    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].frame, 70u);
    EXPECT_EQ(statuses[0].received, d);
    EXPECT_EQ(statuses[0].lost, 0u);
}

TEST(GrUdpProtocol, FrameStatusOnFecRecovery) {
    auto frame = MakeFrameBytes(12000);
    auto meta = MakeMeta(71, true);
    auto pkts = ShardFec(meta, frame, 20);
    const uint16_t d = DataShardsOf(pkts);
    ASSERT_GT(pkts.size(), (size_t)d);

    GrUdpFrameReassembler reasm;
    struct Status { uint32_t frame; uint16_t received; uint16_t lost; };
    std::vector<Status> statuses;
    reasm.on_frame_status_ = [&](uint8_t, uint32_t f, uint16_t r, uint16_t l) {
        statuses.push_back({f, r, l});
    };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };
    // 丢 1 个数据 shard,FEC 恢复:received = D-1,lost = 1,恰好一次
    for (size_t i = 1; i < pkts.size(); i++) {
        reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    }
    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].frame, 71u);
    EXPECT_EQ(statuses[0].received, d - 1);
    EXPECT_EQ(statuses[0].lost, 1u);
}

TEST(GrUdpProtocol, FrameStatusOnDeclareLoss) {
    auto frame_n = MakeFrameBytes(8000);
    auto meta_n = MakeMeta(80, false);
    auto pkts_n = GrUdpProtocol::ShardVideoFrame(meta_n, frame_n.data(), frame_n.size());
    const uint16_t d = DataShardsOf(pkts_n);
    ASSERT_GT(d, 1u);

    auto frame_key = MakeFrameBytes(700);
    auto meta_key = MakeMeta(81, true);
    auto pkts_key = GrUdpProtocol::ShardVideoFrame(meta_key, frame_key.data(), frame_key.size());

    GrUdpFrameReassembler reasm;
    struct Status { uint32_t frame; uint16_t received; uint16_t lost; };
    std::vector<Status> statuses;
    reasm.on_frame_status_ = [&](uint8_t, uint32_t f, uint16_t r, uint16_t l) {
        statuses.push_back({f, r, l});
    };

    // 帧 80 丢最后一个 shard 卡住;帧 81 到达 -> 80 判丢(状态一次:received=D-1, lost=1)
    for (size_t i = 0; i + 1 < pkts_n.size(); i++) reasm.AddPacket(pkts_n[i]->CStr(), pkts_n[i]->Size());
    EXPECT_TRUE(statuses.empty());
    for (auto& p : pkts_key) reasm.AddPacket(p->CStr(), p->Size());
    // 帧 80 判丢一次 + 帧 81 完成一次
    ASSERT_EQ(statuses.size(), 2u);
    EXPECT_EQ(statuses[0].frame, 80u);
    EXPECT_EQ(statuses[0].received, d - 1);
    EXPECT_EQ(statuses[0].lost, 1u);
    EXPECT_EQ(statuses[1].frame, 81u);
    EXPECT_EQ(statuses[1].lost, 0u);

    // 帧 80 的迟到包不得再次触发状态(finished_ 去重)
    reasm.AddPacket(pkts_n.back()->CStr(), pkts_n.back()->Size());
    EXPECT_EQ(statuses.size(), 2u);
}

// ---------------- 整帧丢失 gap 检测 + 恢复校验 ----------------

TEST(GrUdpProtocol, WholeFrameLossDetectedOnNextSof) {
    // 帧 90(key) 完整 -> emit
    auto frame90 = MakeFrameBytes(700);
    auto pkts90 = GrUdpProtocol::ShardVideoFrame(MakeMeta(90, true), frame90.data(), frame90.size());
    // 帧 91 整帧蒸发:生成但一个包都不投
    auto frame91 = MakeFrameBytes(700);
    auto pkts91 = GrUdpProtocol::ShardVideoFrame(MakeMeta(91, false), frame91.data(), frame91.size());
    (void)pkts91;
    // 帧 92(P) 全到:参考链已断,必须被 need_key_ 丢掉
    auto frame92 = MakeFrameBytes(700);
    auto pkts92 = GrUdpProtocol::ShardVideoFrame(MakeMeta(92, false), frame92.data(), frame92.size());
    // 帧 93(key):流恢复
    auto frame93 = MakeFrameBytes(700);
    auto pkts93 = GrUdpProtocol::ShardVideoFrame(MakeMeta(93, true), frame93.data(), frame93.size());

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    std::vector<uint32_t> lost;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };

    for (auto& p : pkts90) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(lost.empty()); // 首连后连续帧,不误判

    // 跳过 91,直接喂 92 -> 检测出 [91] 整帧丢失,DeclareLoss 恰好一次
    for (auto& p : pkts92) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 91u);
    // 92 是 P 帧,need_key_ 置位 -> 完成但不进解码器
    EXPECT_EQ(frames.size(), 1u);

    // key 帧 93 正常 emit,流恢复
    for (auto& p : pkts93) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[1].frame_index_, 93u);
    EXPECT_TRUE(frames[1].key_);
    // 没有重复判丢
    EXPECT_EQ(lost.size(), 1u);
}

TEST(GrUdpProtocol, ContiguousFramesNoFalseGap) {
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };

    // gap = 0 的连续帧:100(key) -> 101(P) -> 102(P),全部正常 emit
    for (uint32_t idx = 100; idx <= 102; idx++) {
        auto frame = MakeFrameBytes(700 + idx);
        auto pkts = GrUdpProtocol::ShardVideoFrame(MakeMeta(idx, idx == 100), frame.data(), frame.size());
        for (auto& p : pkts) reasm.AddPacket(p->CStr(), p->Size());
    }
    EXPECT_EQ(frames.size(), 3u);
}

TEST(GrUdpProtocol, ValidateRecoveredShard0Check) {
    const int d = 9;
    const size_t p = 1376; // mtu 1400 - 24
    // 合法块:ext(9+4) + frame_size 在容量内
    std::string b0(p, '\0');
    GrUdpProtocol::W16(b0.data(), 1920);
    GrUdpProtocol::W16(b0.data() + 2, 1080);
    GrUdpProtocol::W32(b0.data() + 4, 12000);
    b0[8] = 4;
    memcpy(b0.data() + 9, "mon1", 4);
    EXPECT_TRUE(GrUdpFrameReassembler::ValidateRecoveredShard0(b0, d, p));

    // mon_name_len 超上限
    auto bad_nl = b0;
    bad_nl[8] = (char)200;
    EXPECT_FALSE(GrUdpFrameReassembler::ValidateRecoveredShard0(bad_nl, d, p));

    // frame_size 超容量:sof_ext(13) + frame_size > D * P (12384)
    auto bad_size = b0;
    GrUdpProtocol::W32(bad_size.data() + 4, 20000);
    EXPECT_FALSE(GrUdpFrameReassembler::ValidateRecoveredShard0(bad_size, d, p));

    // 块太短
    EXPECT_FALSE(GrUdpFrameReassembler::ValidateRecoveredShard0(std::string(5, '\0'), d, p));
}
