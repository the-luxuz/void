#include "player.h"

namespace PlayerActor {

    static EntityID eyeHeightTrackedPlayerID = 0;
    static float previousCameraEyeHeight = cameraEyeHeight;

    static constexpr float GroundSlopeUpDotThreshold = 0.999f;
    static EntityID viewRotationPlayerID = 0;
    static float playerViewYawAtStart = 0.0f;
    static glm::quat playerRotationAtViewStart = glm::identity<glm::quat>();

    static void AlignPlayerToView(Object &player) {

        glm::vec3 playerForward = player.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 horizontalForward(playerForward.x, 0.0f, playerForward.z);

        if (glm::length2(horizontalForward) > 0.0001f) {

            float playerYaw = std::atan2(horizontalForward.z, horizontalForward.x);
            float yawAdjustment = playerYaw - cameraState.alpha;

            player.rotation = glm::normalize(glm::angleAxis(yawAdjustment, glm::vec3(0.0f, 1.0f, 0.0f)) * player.rotation);
        }

        viewRotationPlayerID = player.id;
        playerViewYawAtStart = cameraState.alpha;
        playerRotationAtViewStart = player.rotation;
    }

    static void RotatePlayerWithView(Object &player) {

        if (viewRotationPlayerID != player.id) {

            AlignPlayerToView(player);

            return;
        }

        float viewYawDelta = cameraState.alpha - playerViewYawAtStart;
        player.rotation = glm::normalize(glm::angleAxis(-viewYawDelta, glm::vec3(0.0f, 1.0f, 0.0f)) * playerRotationAtViewStart);
    }

    static glm::vec3 GetDownhillDirection(const Physics &physics, const glm::vec3 &surfaceNormal) {

        glm::vec3 gravity = physics.useGravity ? physics.gravity : glm::vec3(0.0f);
        glm::vec3 normal = glm::normalize(surfaceNormal);
        glm::vec3 downhill = gravity - normal * glm::dot(gravity, normal);

        return glm::length2(downhill) > 0.0001f ? glm::normalize(downhill) : glm::vec3(0.0f);
    }

    static glm::vec3 GetDownhillDirection(const Physics &physics) {

        return GetDownhillDirection(physics, groundNormal);
    }

    static float GetSlopeHorizontalSpeedLimit(const Physics &physics, const glm::vec3 &surfaceNormal, float maxTangentSpeed) {

        glm::vec3 normal = glm::normalize(surfaceNormal);
        glm::vec3 tangentVelocity = physics.velocity - normal * glm::dot(physics.velocity, normal);

        float tangentSpeed = glm::length(tangentVelocity);

        glm::vec3 tangentDirection = tangentSpeed > 0.001f ? tangentVelocity / tangentSpeed : GetDownhillDirection(physics, normal);

        float horizontalShare = glm::length(glm::vec2(tangentDirection.x, tangentDirection.z));

        if (horizontalShare <= 0.001f)
            return maxTangentSpeed;

        return std::max(maxTangentSpeed, tangentSpeed) * horizontalShare;
    }

    static void CounteractHorizontalDrag(Object &player, float dt) {

        Physics &physics = player.physics;

        if (dt <= 0.0f || physics.inverseMass <= 0.0f)
            return;

        glm::vec3 downhillDirection = GetDownhillDirection(physics);

        if (glm::length2(downhillDirection) <= 0.0001f || glm::dot(physics.velocity, downhillDirection) <= 0.0f)
            return;

        float dragFactor = glm::clamp(1.0f - physics.drag * dt, 0.0f, 1.0f);

        if (dragFactor <= 0.0001f)
            return;

        glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);
        glm::vec3 acceleration = (horizontalVelocity / dragFactor - horizontalVelocity) / dt;

        AddForce(player, acceleration / physics.inverseMass);
    }

    void ResetJumpState() {

        jumpActionWasDown = false;

        grounded = false;
        groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        groundNormalUpDot = 0.0f;
        rampUphillBlock = false;
        rampUphillDownhillDirection = glm::vec3(0.0f);

        jumpBufferRemaining = 0.0f;

        shiftWasDown = false;
        slideInputSuppressedUntilRelease = false;
        movingBeforeSlide = false;
        sliding = false;
        normalMaximumSpeed = 0.0f;
        slideCurrentSpeed = 0.0f;
        slideSpeedCeiling = 0.0f;
        slideDirection = glm::vec3(0.0f);
        slideVelocityDirection = glm::vec3(0.0f);
        slideCarryActive = false;
        slideCarrySpeed = 0.0f;
        slideCarryDirection = glm::vec3(0.0f);
        slideCarryLandingDecayRemaining = 0.0f;
        slideCarryLandingDecayStarted = false;
        slideCarryLandingStartSpeed = 0.0f;
        slideJumpReleaseWindowRemaining = 0.0f;
        releasedSlideSpeed = 0.0f;
        releasedSlideDirection = glm::vec3(0.0f);
        crouched = false;
        crouchAmount = 0.0f;
        standingScaleInitialized = false;
        crouchPlayerID = 0;
        standingScale = glm::vec3(1.0f);
        speedFovOffset = 0.0f;

        coyoteRemaining = 0.0f;
    }

    void QueueJumpInput() {

        jumpBufferRemaining = jumpBufferDuration;
    }

    void BeginCollisionStep() {

        grounded = false;
        groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        groundNormalUpDot = 0.0f;
        rampUphillBlock = false;
    }

    static void RegisterCollisionObjects(Object &a, Object &b, const Contact &contact) {

        if (!contact.hit || a.collider.isTrigger || b.collider.isTrigger)
            return;

        glm::vec3 up(0.0f, 1.0f, 0.0f);
        bool playerCollision = a.name == "Player" || b.name == "Player";
        glm::vec3 supportNormal(0.0f);

        Object *playerObject = nullptr;

        if (a.name == "Player") {

            playerObject = &a;
            supportNormal = -contact.normal;

        } else if (b.name == "Player") {

            playerObject = &b;
            supportNormal = contact.normal;
        }

        float supportUpDot = glm::dot(supportNormal, up);

        bool wallCollision = std::abs(glm::dot(contact.normal, up)) < 0.1f;

        bool nonWalkableSlopeContact = playerCollision && supportUpDot < groundNormalThreshold && std::abs(supportUpDot) < 0.9999f;
        bool hitUndersideOfSlope = nonWalkableSlopeContact && supportUpDot < 0.0f;

        glm::vec3 downhillDirection(0.0f);

        if (hitUndersideOfSlope && playerObject && playerObject->hasPhysics)
            downhillDirection = GetDownhillDirection(playerObject->physics, supportNormal);

        if (playerCollision && (wallCollision || hitUndersideOfSlope)) {

            if (sliding && !shiftWasDown)
                sliding = false;

            if (hitUndersideOfSlope) {

                sliding = false;

                slideInputSuppressedUntilRelease = true;
            }

            movingBeforeSlide = false;
            slideCurrentSpeed = 0.0f;
            slideSpeedCeiling = 0.0f;
            slideCarryActive = false;
            slideCarrySpeed = 0.0f;
            slideCarryDirection = glm::vec3(0.0f);
            slideCarryLandingDecayRemaining = 0.0f;
            slideCarryLandingDecayStarted = false;
            slideCarryLandingStartSpeed = 0.0f;
            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

            if (hitUndersideOfSlope && playerObject && playerObject->hasPhysics) {

                if (glm::length2(downhillDirection) <= 0.0001f)
                    downhillDirection = GetDownhillDirection(playerObject->physics, supportNormal);

                rampUphillBlock = true;
                rampUphillDownhillDirection = downhillDirection;
            }

            if (nonWalkableSlopeContact && playerObject && playerObject->hasPhysics && hitUndersideOfSlope) {

                float velocityAlongDownhill = glm::dot(playerObject->physics.velocity, downhillDirection);

                if (velocityAlongDownhill < 0.0f)
                    playerObject->physics.velocity -= downhillDirection * velocityAlongDownhill;
            }
        }

        if (supportUpDot > groundNormalThreshold && supportUpDot > groundNormalUpDot) {

            grounded = true;
            groundNormal = glm::normalize(supportNormal);
            groundNormalUpDot = supportUpDot;
        }
    }

    void RegisterCollision(EntityID aID, EntityID bID, const Contact &contact) {

        Object *a = FindObjectByID(aID);
        Object *b = FindObjectByID(bID);

        if (!a || !b)
            return;

        RegisterCollisionObjects(*a, *b, contact);
    }

    static void ControlObject(const PlayerInput &input, float dt, Object &player) {

        bool jumpActionDown = input.jump;

        if (jumpActionDown && !jumpActionWasDown)
            QueueJumpInput();
        else if (jumpBufferRemaining > 0.0f)
            jumpBufferRemaining = std::max(0.0f, jumpBufferRemaining - dt);

        jumpActionWasDown = jumpActionDown;

        if (!player.hasPhysics || player.physics.isStatic)
            return;

        Physics &physics = player.physics;

        if (physics.inverseMass <= 0.0f)
            return;

        bool groundedOnSlopeAtControlStart = grounded && groundNormal.y < GroundSlopeUpDotThreshold;

        if (groundedOnSlopeAtControlStart && (sliding || slideCarryActive))
            physics.velocity -= groundNormal * glm::dot(physics.velocity, groundNormal);

        if (rampUphillBlock && glm::length2(rampUphillDownhillDirection) > 0.0001f) {

            float velocityAlongDownhill = glm::dot(physics.velocity, rampUphillDownhillDirection);

            if (velocityAlongDownhill < 0.0f)
                physics.velocity -= rampUphillDownhillDirection * velocityAlongDownhill;
        }

        if (!standingScaleInitialized || crouchPlayerID != player.id) {

            standingScale = player.scale;
            standingScaleInitialized = true;
            crouchPlayerID = player.id;
        }

        if (physics.maxHorizontalSpeed <= 0.0f)
            physics.maxHorizontalSpeed = defaultMaximumSpeed;

        float cosYaw = std::cos(cameraState.alpha);
        float sinYaw = std::sin(cameraState.alpha);

        glm::vec3 forward(cosYaw, 0.0f, sinYaw);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 moveDirection(0.0f);

        if (input.moveForward)
            moveDirection += forward;

        if (input.moveBackward)
            moveDirection -= forward;

        if (input.moveLeft)
            moveDirection -= right;

        if (input.moveRight)
            moveDirection += right;

        if (rampUphillBlock) {

            glm::vec3 downhillHorizontal(rampUphillDownhillDirection.x, 0.0f, rampUphillDownhillDirection.z);

            if (glm::length2(downhillHorizontal) > 0.0001f) {

                downhillHorizontal = glm::normalize(downhillHorizontal);

                float inputAlongDownhill = glm::dot(moveDirection, downhillHorizontal);

                if (inputAlongDownhill < 0.0f)
                    moveDirection -= downhillHorizontal * inputAlongDownhill;
            }
        }

        //-----------------------------------------------------------------------------
        // slide

        bool shiftDown = input.slide;
        bool shiftPressed = shiftDown && !shiftWasDown;
        bool hasMove = glm::length2(moveDirection) > 0.0001f;
        bool slideWasActive = sliding;

        float previousSlideCurrentSpeed = slideCurrentSpeed;

        glm::vec3 previousSlideVelocityDirection = slideVelocityDirection;

        if (shiftPressed)
            movingBeforeSlide = hasMove;

        shiftWasDown = shiftDown;

        if (!shiftDown) {
            slideInputSuppressedUntilRelease = false;
            crouched = false;
        } else if (slideInputSuppressedUntilRelease)
            crouched = false;
        else if (grounded)
            crouched = true;

        if (shiftDown && !grounded && physics.velocity.y <= 0.0f)
            AddForce(player, glm::vec3(0.0f, (physics.gravity.y * physics.inverseMass * 8.f), 0.0f));

        float previousAmount = crouchAmount;
        float targetAmount = crouched ? 1.0f : 0.0f;
        float step = dt / std::max(crouchTransitionDuration, 0.001f);

        crouchAmount += glm::clamp(targetAmount - crouchAmount, -step, step);

        player.scale = standingScale;
        player.scale.y = standingScale.y * (1.0f - (1.0f - slideCrouchScale) * crouchAmount);

        player.position.y -= std::abs(standingScale.y) * (1.0f - slideCrouchScale) * 0.5f * (crouchAmount - previousAmount);

        if (normalMaximumSpeed <= 0.0f)
            normalMaximumSpeed = physics.maxHorizontalSpeed;

        if (shiftDown && !slideInputSuppressedUntilRelease && grounded && movingBeforeSlide && hasMove && !sliding) {

            sliding = true;
            slideCarryActive = false;

            slideCarrySpeed = 0.0f;

            slideCarryDirection = glm::vec3(0.0f);
            slideCarryLandingDecayRemaining = 0.0f;

            slideCarryLandingDecayStarted = false;
            slideCarryLandingStartSpeed = 0.0f;
            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

            glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

            slideDirection = glm::length2(horizontalVelocity) > 0.0001f ? glm::normalize(horizontalVelocity) : glm::normalize(moveDirection);

            float currentSpeed = glm::length(horizontalVelocity);
            bool onSlope = grounded && groundNormal.y < GroundSlopeUpDotThreshold;

            glm::vec3 requestedDirection = moveDirection;

            if (glm::length2(requestedDirection) > 0.0001f)
                requestedDirection = glm::normalize(requestedDirection);

            glm::vec3 downhillDirection = GetDownhillDirection(physics);

            bool uphillIntent = onSlope && glm::length2(downhillDirection) > 0.0001f && (glm::dot(physics.velocity, downhillDirection) < -0.001f || glm::dot(requestedDirection, downhillDirection) < -0.001f);

            slideCurrentSpeed = uphillIntent ? currentSpeed : std::max(currentSpeed, normalMaximumSpeed * 1.5f);

            slideSpeedCeiling = slideCurrentSpeed;

            slideVelocityDirection = slideDirection;

        }

        if (!shiftDown) {

            sliding = false;

            movingBeforeSlide = false;

            slideCurrentSpeed = 0.0f;

            slideSpeedCeiling = 0.0f;
        }

        if (shiftDown) {

            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

        } else if (slideWasActive) {

            slideJumpReleaseWindowRemaining = std::max(slideJumpReleaseWindowDuration, 0.0f);
            releasedSlideSpeed = previousSlideCurrentSpeed;
            releasedSlideDirection = previousSlideVelocityDirection;

        } else if (slideJumpReleaseWindowRemaining > 0.0f)
            slideJumpReleaseWindowRemaining = std::max(0.0f, slideJumpReleaseWindowRemaining - dt);

        if (slideJumpReleaseWindowRemaining <= 0.0f) {

            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);
        }

        float referenceSpeed = std::max(normalMaximumSpeed * speedFovReferenceMultiplier, 1.0f);

        glm::vec2 horizontalVelocity(physics.velocity.x, physics.velocity.z);

        float speedRatio = glm::clamp(glm::length(horizontalVelocity) / referenceSpeed, 0.0f, 1.0f);
        float targetFovOffset = speedFovIncrease * speedRatio;
        float maxFovStep = speedFovChangeSpeed * dt;

        speedFovOffset += glm::clamp(targetFovOffset - speedFovOffset, -maxFovStep, maxFovStep);

        if (grounded)
            coyoteRemaining = coyoteDuration;
        else
            coyoteRemaining = std::max(0.0f, coyoteRemaining - dt);

        //-----------------------------------------------------------------------------
        // jump

        float downwardGravity = -physics.gravity.y * jumpGravityMultiplier;

        bool jumpLaunchedThisStep = false;

        // jump again
        if (!sliding && coyoteRemaining > 0.0f && jumpBufferRemaining > 0.0f) {

            float launchSpeed = std::sqrt(2.0f * downwardGravity * std::max(jumpHeight, 0.0f));

            physics.velocity.y = launchSpeed;

            glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

            slideCarrySpeed = glm::length(horizontalVelocity);
            slideCarryDirection = slideCarrySpeed > 0.001f ? glm::normalize(horizontalVelocity) : slideVelocityDirection;

            float slideSpeedForJump = !shiftDown ? releasedSlideSpeed : 0.0f;
            glm::vec3 slideDirectionForJump = releasedSlideDirection;

            if (slideSpeedForJump > slideCarrySpeed) {

                slideCarrySpeed = slideSpeedForJump;
                slideCarryDirection = slideDirectionForJump;
            }

            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

            slideCarryActive = slideCarrySpeed > 0.001f;

            sliding = false;

            jumpBufferRemaining = 0.0f;

            coyoteRemaining = 0.0f;

            grounded = false;
            jumpLaunchedThisStep = true;
        }

        if (slideCarryActive) {

            if (!grounded || jumpLaunchedThisStep) {

                slideCarryLandingDecayRemaining = 0.0f;
                slideCarryLandingDecayStarted = false;
                slideCarryLandingStartSpeed = 0.0f;

            } else if (groundNormal.y < GroundSlopeUpDotThreshold) {

                slideCarryLandingDecayRemaining = 0.0f;
                slideCarryLandingDecayStarted = false;
                slideCarryLandingStartSpeed = 0.0f;

            } else if (!sliding) {

                float decayDuration = std::max(slideCarryLandingDecayDuration, 0.0f);

                if (!slideCarryLandingDecayStarted) {

                    slideCarryLandingDecayRemaining = decayDuration;
                    slideCarryLandingDecayStarted = true;

                    glm::vec2 horizontalVelocity(physics.velocity.x, physics.velocity.z);

                    slideCarryLandingStartSpeed = std::max(slideCarrySpeed, glm::length(horizontalVelocity));

                } else
                    slideCarryLandingDecayRemaining = std::max(0.0f, slideCarryLandingDecayRemaining - dt);

                float landingProgress = decayDuration > 0.0f ? 1.0f - slideCarryLandingDecayRemaining / decayDuration : 1.0f;

                landingProgress = glm::clamp(landingProgress, 0.0f, 1.0f);
                landingProgress = landingProgress * landingProgress * (3.0f - 2.0f * landingProgress);

                slideCarrySpeed = glm::mix(slideCarryLandingStartSpeed, normalMaximumSpeed, landingProgress);

                if (slideCarryLandingDecayRemaining <= 0.0f) {

                    slideCarryActive = false;

                    slideCarrySpeed = 0.0f;

                    slideCarryDirection = glm::vec3(0.0f);

                    slideCarryLandingDecayStarted = false;
                    slideCarryLandingStartSpeed = 0.0f;
                }
            }
        }

        if (!grounded && physics.useGravity && jumpGravityMultiplier > 1.0f) {

            float extraGravityForce = -physics.gravity.y * (jumpGravityMultiplier - 1.0f) / physics.inverseMass;

            AddForce(player, glm::vec3(0.0f, -extraGravityForce, 0.0f));
        }

        if (glm::length2(moveDirection) > 0.0001f)
            moveDirection = glm::normalize(moveDirection);

        if (sliding && hasMove) {

            float maxTurn = glm::radians(60.0f) * dt;
            float dot = glm::clamp(glm::dot(slideVelocityDirection, moveDirection), -1.0f, 1.0f);

            float angle = std::atan2(slideVelocityDirection.z * moveDirection.x - slideVelocityDirection.x * moveDirection.z, dot);

            angle = glm::clamp(angle, -maxTurn, maxTurn);

            float c = std::cos(angle);
            float s = std::sin(angle);
            glm::vec3 oldDirection = slideVelocityDirection;

            slideVelocityDirection = glm::normalize(glm::vec3(c * oldDirection.x + s * oldDirection.z, 0.0f, -s * oldDirection.x + c * oldDirection.z));
        }

        if (sliding) {

            float turnFactor = 1.0f;

            if (hasMove) {

                float alignment = glm::clamp(glm::dot(slideDirection, slideVelocityDirection), -1.0f, 1.0f);

                float alignment01 = (alignment + 1.0f) * 0.5f;
                turnFactor = glm::mix(0.4f, 1.0f, alignment01);

                if (grounded && groundNormal.y >= GroundSlopeUpDotThreshold)
                    slideSpeedCeiling = std::max(0.0f, slideSpeedCeiling - normalMaximumSpeed * 0.50f * dt);

                float targetSpeed = slideSpeedCeiling * turnFactor;

                float response = normalMaximumSpeed * 2.0f * dt;

                slideCurrentSpeed += glm::clamp(targetSpeed - slideCurrentSpeed, -response, response);

            } else if (grounded && groundNormal.y >= GroundSlopeUpDotThreshold)
                slideCurrentSpeed = std::max(0.0f, slideCurrentSpeed - normalMaximumSpeed * 2.5f * dt);

            bool groundedOnSlope = grounded && groundNormal.y < GroundSlopeUpDotThreshold;

            if (groundedOnSlope) {

                glm::vec3 downhillDirection = GetDownhillDirection(physics);
                glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

                float currentHorizontalSpeed = glm::length(horizontalVelocity);

                slideCurrentSpeed = currentHorizontalSpeed;
                slideSpeedCeiling = currentHorizontalSpeed;

                if (currentHorizontalSpeed > 0.001f)
                    slideVelocityDirection = horizontalVelocity / currentHorizontalSpeed;
                else {

                    glm::vec3 downhillHorizontal(downhillDirection.x, 0.0f, downhillDirection.z);

                    if (glm::length2(downhillHorizontal) > 0.0001f)
                        slideVelocityDirection = glm::normalize(downhillHorizontal);
                }
            }

            physics.maxHorizontalSpeed = groundedOnSlope ? GetSlopeHorizontalSpeedLimit(physics, groundNormal, normalMaximumSpeed * 2.0f) : normalMaximumSpeed * 2.0f;

            if (groundedOnSlope) 
                CounteractHorizontalDrag(player, dt);
            else if (dt > 0.0f) {

                float dragFactor = std::max(1.0f - physics.drag * dt, 0.01f);

                glm::vec3 currentVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

                glm::vec3 targetVelocity = slideVelocityDirection * slideCurrentSpeed;

                glm::vec3 acceleration = (targetVelocity / dragFactor - currentVelocity) / dt;

                AddForce(player, acceleration / physics.inverseMass);
            }

        } else if (slideCarryActive && (!grounded || slideCarryLandingDecayRemaining > 0.0f || groundNormal.y < GroundSlopeUpDotThreshold)) {

            if (!grounded && hasMove && glm::length2(slideCarryDirection) > 0.0001f) {

                float maxTurn = glm::radians(slideAirTurnDegreesPerSecond) * dt;

                float dot = glm::clamp(glm::dot(slideCarryDirection, moveDirection), -1.0f, 1.0f);

                float angle = std::atan2(slideCarryDirection.z * moveDirection.x - slideCarryDirection.x * moveDirection.z, dot);
                angle = glm::clamp(angle, -maxTurn, maxTurn);

                float c = std::cos(angle);
                float s = std::sin(angle);

                glm::vec3 oldDirection = slideCarryDirection;

                slideCarryDirection = glm::normalize(glm::vec3(c * oldDirection.x + s * oldDirection.z, 0.0f, -s * oldDirection.x + c * oldDirection.z));
            }

            bool groundedOnSlope = grounded && groundNormal.y < GroundSlopeUpDotThreshold;

            if (groundedOnSlope) {

                glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

                slideCarrySpeed = glm::length(horizontalVelocity);

                if (slideCarrySpeed > 0.001f)
                    slideCarryDirection = horizontalVelocity / slideCarrySpeed;
                else {

                    glm::vec3 downhillDirection = GetDownhillDirection(physics);
                    glm::vec3 downhillHorizontal(downhillDirection.x, 0.0f, downhillDirection.z);

                    if (glm::length2(downhillHorizontal) > 0.0001f)
                        slideCarryDirection = glm::normalize(downhillHorizontal);
                }
            }

            physics.maxHorizontalSpeed = groundedOnSlope ? GetSlopeHorizontalSpeedLimit(physics, groundNormal, normalMaximumSpeed * 2.0f) : std::max(normalMaximumSpeed, slideCarrySpeed);

            if (groundedOnSlope)
                CounteractHorizontalDrag(player, dt);
            else if (dt > 0.0f && slideCarrySpeed > 0.001f) {

                float dragFactor = std::max(1.0f - physics.drag * dt, 0.01f);

                glm::vec3 currentVelocity(physics.velocity.x, 0.0f, physics.velocity.z);
                glm::vec3 targetVelocity = slideCarryDirection * slideCarrySpeed;
                glm::vec3 acceleration = (targetVelocity / dragFactor - currentVelocity) / dt;

                AddForce(player, acceleration / physics.inverseMass);
            }

        } else {

            float crouchSpeedFactor = glm::mix(1.0f, crouchedMoveSpeedMultiplier, crouchAmount);
            physics.maxHorizontalSpeed = normalMaximumSpeed * crouchSpeedFactor;

            if (hasMove) {

                float drag = std::max(physics.drag, 1.0f);
                float dragFactor = std::max(1.0f - drag * dt, 0.01f);
                float driveAcceleration = physics.maxHorizontalSpeed * drag / dragFactor;

                glm::vec3 driveForce = moveDirection * (driveAcceleration / physics.inverseMass);

                AddForce(player, driveForce);
            }
        }
    }

    void Control(EntityID playerID, const PlayerInput &input, float dt) {

        Object *playerObject = FindObjectByID(playerID);

        if (!playerObject)
            return;

        ControlObject(input, dt, *playerObject);
    }

    static void UpdateCameraObject(Object &player, float dt) {

        constexpr float maxPitch = glm::radians(89.0f);

        glm::vec3 playerBoundsMin;
        glm::vec3 playerBoundsMax;

        GetWorldBounds(player, playerBoundsMin, playerBoundsMax);

        float playerWorldHeight = playerBoundsMax.y - playerBoundsMin.y;
        float eyeHeightRatio = glm::clamp(cameraEyeHeight / 2.0f, 0.05f, 0.98f);
        float eyeY = playerWorldHeight > 0.001f ? playerBoundsMin.y + playerWorldHeight * eyeHeightRatio : player.position.y + cameraEyeHeight;

        glm::vec3 desiredPosition = glm::vec3(player.position.x, eyeY, player.position.z);

        if (eyeHeightTrackedPlayerID != player.id) {

            eyeHeightTrackedPlayerID = player.id;
            previousCameraEyeHeight = cameraEyeHeight;

        } else if (!cameraInitialized)
            previousCameraEyeHeight = cameraEyeHeight;
        else if (cameraInitialized && cameraEyeHeight != previousCameraEyeHeight) {

            float heightPerEyeUnit = playerWorldHeight * 0.5f;

            cameraState.cam_position.y += (cameraEyeHeight - previousCameraEyeHeight) * heightPerEyeUnit;
            previousCameraEyeHeight = cameraEyeHeight;
        }

        if (!cameraInitialized) {

            glm::vec3 editorPosition = get_cam_pos();
            glm::vec3 editorLook = glm::normalize(cam.cam_target - editorPosition);

            cameraState.alpha = std::atan2(editorLook.z, editorLook.x);
            cameraState.beta = std::asin(glm::clamp(editorLook.y, -1.0f, 1.0f));
            cameraState.beta = glm::clamp(cameraState.beta, -maxPitch, maxPitch);
            cameraState.cam_position = desiredPosition;

            AlignPlayerToView(player);

            cameraInitialized = true;

        } else {

            if (viewRotationPlayerID != player.id)
                AlignPlayerToView(player);

            constexpr float maxCameraLag = 0.12f;

            float playerSpeed = player.hasPhysics ? glm::length(player.physics.velocity) : 0.0f;
            float followSpeed = std::max(cameraFollowSpeed, playerSpeed / maxCameraLag);
            float followAmount = 1.0f - std::exp(-followSpeed * std::max(dt, 0.0f));

            cameraState.cam_position = glm::mix(cameraState.cam_position, desiredPosition, followAmount);
        }

        RotatePlayerWithView(player);

        glm::vec3 lookDirection(std::cos(cameraState.alpha) * std::cos(cameraState.beta), std::sin(cameraState.beta), std::sin(cameraState.alpha) * std::cos(cameraState.beta));

        cameraState.cam_target = cameraState.cam_position + lookDirection;
        cameraState.cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void UpdateCamera(EntityID playerID, float dt) {

        Object *playerObject = FindObjectByID(playerID);

        if (!playerObject)
            return;

        UpdateCameraObject(*playerObject, dt);
    }

    

}

