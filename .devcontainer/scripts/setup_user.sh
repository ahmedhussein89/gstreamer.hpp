#!/bin/bash
set -e

USERNAME="$1"
USER_UID="$2"
USER_GID="$3"

# Check if UID already exists and remove the user
EXISTING_USER=$(getent passwd $USER_UID | cut -d: -f1 || true)
if [ ! -z "$EXISTING_USER" ] && [ "$EXISTING_USER" != "$USERNAME" ]; then
    echo "Removing existing user with UID $USER_UID: $EXISTING_USER"
    userdel -r $EXISTING_USER 2>/dev/null || userdel $EXISTING_USER 2>/dev/null || true
fi

# Check if GID already exists and remove the group
EXISTING_GROUP=$(getent group $USER_GID | cut -d: -f1 || true)
if [ ! -z "$EXISTING_GROUP" ] && [ "$EXISTING_GROUP" != "$USERNAME" ]; then
    echo "Removing existing group with GID $USER_GID: $EXISTING_GROUP"
    groupdel $EXISTING_GROUP 2>/dev/null || true
fi

# Create new group and user
groupadd --gid $USER_GID $USERNAME
useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME

# Setup sudo access
mkdir -p /etc/sudoers.d
echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME
chmod 0440 /etc/sudoers.d/$USERNAME

echo "User $USERNAME created successfully with UID $USER_UID and GID $USER_GID"
