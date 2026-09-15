# =====================================================================
# QtEasyTier CNB AUR 更新流水线构建环境
#
# AUR 更新只需要 git（通过 SSH 推送）+ curl（下载 release 产物），
# 不依赖 Qt 等重型环境，因此单独维护一个极简镜像；仍走 CNB 的
# docker.build 动态构建机制（按 Dockerfile 哈希缓存到制品库，命中即复用）。
# =====================================================================

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      bash ca-certificates coreutils curl findutils git openssh-client \
    && rm -rf /var/lib/apt/lists/*

# 保证中文输出与 git/ssh 行为一致
ENV LANG=C.UTF-8
