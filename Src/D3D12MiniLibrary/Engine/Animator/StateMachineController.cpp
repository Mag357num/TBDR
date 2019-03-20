#include <DirectXMath.h>
#include <memory>
#include <algorithm>
#include "StateMachineController.h"
#include "TransitionConditions.h"
#include "../Model/MMDModel/MMDModelResourceData.h"
#include "../Util/Math.h"
#include "../Util/Utility.h"


K3D12::StateMachineController::StateMachineController()
{
}


K3D12::StateMachineController::~StateMachineController()
{
}


void K3D12::StateMachineController::CCDIKSolver(K3D12::MMDIKData & ikData)
{
	Vector3 ikEffecterPos = _skelton.lock()->bonesMatrix[ikData.boneIndex].axisW; //�A�j���[�V�����̌�ɍs���\��Ȃ̂Ō��܂��Ă���͂��i�^�[�Q�b�g�{�[�����ڎw���ړI�n�H�j
	Vector3 ikTargetBonePos = _skelton.lock()->bonesMatrix[ikData.targetBoneIndex].axisW; //IK�̃^�[�Q�b�g�{�[���̈ʒu

	std::vector<Vector3> tempLocationMat;
	tempLocationMat.reserve(ikData.ikLinks.size());
	for (auto& ikLink : ikData.ikLinks) {
		tempLocationMat.push_back(_skelton.lock()->bonesMatrix[ikLink.boneIndex].axisW);
	}

	Vector3 ikEffecterRootVec = ikEffecterPos - tempLocationMat[ikData.ikLinks.size() - 1];
	Vector3 ikTargetBoneRootVec = ikTargetBonePos - tempLocationMat[ikData.ikLinks.size() - 1];
	float   ikMaxLength = ikEffecterRootVec.Length();
	//���[�v�O��]�v�Z
	{

	}

	int chainCount = static_cast<int>(ikData.ikLinks.size());
	for (int count = 0; count < ikData.loopCount; ++count) {
		//�������^�[�Q�b�g�ʒu��IK�{�[���ʒu�Ɠ����Ȃ�΂��̏������I������
		for (int i = 0; i < ikData.ikLinks.size(); i++) {

			if (ikEffecterPos == ikTargetBonePos) {
				break;
			}

			//���݂̃{�[���ʒu����IK�G�t�F�N�^�A����у^�[�Q�b�g�{�[���ւ̃x�N�g�����쐬
			Vector3 originVec = ikEffecterPos - tempLocationMat[i];
			Vector3 transVec = ikTargetBonePos - tempLocationMat[i];

			if (abs(originVec.Length()) <= 0.0001f || abs(transVec.Length()) <= 0.0001f) {
				return;
			}
			//��]���̌v�Z
			Vector3 axis = Vector3::Cross(Vector3::Normalize(originVec), Vector3::Normalize(transVec));

			if (_skelton.lock()->boneNameAccessor[ikData.ikLinks[i].boneIndex].find("�Ђ�") != std::string::npos ||
				_skelton.lock()->boneNameAccessor[ikData.ikLinks[i].boneIndex].find("knee") != std::string::npos)
			{
				//��]����X��
				axis = Vector3::left;
			}
			else {
				if (axis.Length() == 0) {
					return;
				}
			}

			//��p�̂Ȃ��p���v�Z
			auto betweenRot = Vector3::ComputeCrossingAngle(Vector3::Normalize(originVec), Vector3::Normalize(transVec));
			if (abs(betweenRot) == 0.0f) {
				return;
			}

			//�p�x�������`�F�b�N
			{
				//��������N�����v(��������IK���e�����Ƃ̊p�x������p����Ȃ�)
				if (ikData.ikLinks[i].hasLimit) {


				}
				//
				//betweenRot = ::Clamp(betweenRot, -ikData.limitRadian, ikData.limitRadian);
				//���[�v���̐����K�p
				betweenRot = ::Clamp(betweenRot, 0.0f, ikData.limitRadian);
			}

			//�C�ӎ���]���s���l�������쐬
			Quaternion rot = Quaternion::CreateFromAxisAngles(Vector3::Normalize(axis), betweenRot);
			//�{�[�����猴�_�ւ̃x�N�g��
			Vector3 offset = _skelton.lock()->bonesMatrix[ikData.ikLinks[i].boneIndex].axisW;
			//�����s��@-T*R*T
			Matrix afinMat = Matrix::CreateTranslationMatrix(-offset) * Matrix::CreateFromQuaternion(rot) * Matrix::CreateTranslationMatrix(offset);

			_skelton.lock()->bonesMatrix[ikData.ikLinks[i].boneIndex] *= afinMat;

			//
			offset = tempLocationMat[i];
			//�I�t�Z�b�g�l���Ⴄ���Ƃɗ���
			afinMat = Matrix::CreateTranslationMatrix(-offset) * Matrix::CreateFromQuaternion(rot) * Matrix::CreateTranslationMatrix(offset);

			for (int j = i - 1; j >= 0; --j) {
				tempLocationMat[j] = Vector3::Transform(tempLocationMat[j], afinMat);
			}
		}
	}
}

void K3D12::StateMachineController::SetModelSkelton(std::weak_ptr<K3D12::MMDBoneTree> skelton)
{
	this->_skelton = skelton;
}

void K3D12::StateMachineController::ResetSkelton()
{
	std::fill(_skelton.lock()->bonesMatrix.begin(), _skelton.lock()->bonesMatrix.end(), Matrix::CreateIdentity());
}

//�{�[�����̂̕ό`�i�u�����h�͍l�����Ȃ��j
void K3D12::StateMachineController::PlayAnimation(unsigned int& time)
{

	for (auto& frames : _currentNode.lock()->_animation.lock()->keyFrame) {
		//�C���f�b�N�X�̐ݒ�
		if (!_skelton.expired()) {
			unsigned int index = _skelton.lock()->boneAccessor[frames.first]->index;

			//���݂���ׂ��{�[���̃L�[�t���[���̌��� �i���׍팸�ł��邩�炠�Ƃł����������R�[�h�����āj
			auto rbegin = frames.second.rbegin();
			auto rend = frames.second.rend();
			auto revIt = std::find_if(frames.second.rbegin(), frames.second.rend(), [&](K3D12::MotionKeyFrameData& md) { return md.frameNo <= _currentNode.lock()->_animation.lock()->frame; });
			//�⊮�W�����Z�o
			float t;
			//0div���l��


			if (revIt != frames.second.rbegin()) {
				auto a = revIt.base()->frameNo;
				t = static_cast<float>((_currentNode.lock()->_animation.lock()->frame - revIt->frameNo)) / static_cast<float>((revIt.base()->frameNo - revIt->frameNo));
			}
			else {
				t = 0.0f;
			}
			//��]���
			Matrix rotationMat;
			Matrix locationMat;
			if (!(revIt != frames.second.rbegin())) {
				rotationMat = Matrix::CreateFromQuaternion(revIt->quaternion);
				locationMat = Matrix::CreateTranslationMatrix(revIt->location);
			}
			else {
				rotationMat = Matrix::CreateFromQuaternion(Quaternion::Slerp(revIt->quaternion, revIt.base()->quaternion, t));
				locationMat = Matrix::CreateTranslationMatrix(Vector3::Lerp(revIt->location, revIt.base()->location,t));
			}

			//��Ŋ֐���

			{
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTION_CONNECTION) {
				//	//�������Ȃ�
				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_ROTATABLE) {
				//	if (!(revIt != frames.second.rbegin())) {
				//		rotationMat = Matrix::CreateFromQuaternion(revIt->quaternion);
				//	}
				//	else {
				//		rotationMat = Matrix::CreateFromQuaternion(Quaternion::Slerp(revIt->quaternion, revIt.base()->quaternion, t));
				//	}
				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_MOVABLE) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_DISPLAY) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_CONTROLLABLE) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_GRANTL_LOCAL) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_GRANT_ROTATE) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_GRANT_MOVE) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_AXIS_FIXING) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_AXIS_LOCAL) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_AFTER_PHYSICS) {

				//}
				//if (_skelton.lock()->boneAccessor[frames.first]->boneOptionFlags & BONE_OPTIONS::BONE_OPTIONS_EXTERNAL_PARENT) {

				//}
			}

			//���_�ւ̃x�N�g��
			Vector3 offsetToOrigin = _skelton.lock()->boneAccessor[frames.first]->pos;
			Matrix bone = {};
			bone = Matrix::CreateTranslationMatrix(-offsetToOrigin);
			//�C�ӎ���]�i�N�H�^�[�j�I���j����]�s��֒����Ă���
			bone *= rotationMat;
			bone *= Matrix::CreateTranslationMatrix(offsetToOrigin);

			_skelton.lock()->bonesMatrix[index] = bone;
		}
	}
	//�ύX�K�p
	ApplyRecursionMatirixBone(&_skelton.lock()->rootBone);
	for (auto& ik : _skelton.lock()->modelIKdata) {
		CCDIKSolver(ik);
	}
	++time;
	if (++_currentNode.lock()->_animation.lock()->frame >= _currentNode.lock()->_animation.lock()->vmdMotion.back().frameNo) {
		_currentNode.lock()->_animation.lock()->frame = 0;
	}

}

void K3D12::StateMachineController::AttachAnimation(std::string animationName, std::weak_ptr<K3D12::MotionData> motionData)
{
	if (_stateMap.find(animationName) != _stateMap.end()) {
		//���łɂ���
		return;
	}

	std::shared_ptr<StateNode>animationNode = std::make_shared<StateNode>();
	animationNode->_animation = motionData;
	animationNode->_stateName = animationName;
	_stateMap[animationName] = animationNode;
	if (_stateMap.size() == 1) {
		_currentNode = _stateMap[animationName];
	}
}

void K3D12::StateMachineController::ConnectingAnimation(std::string from, std::string to, K3D12::TransitionConditions conditions)
{

	assert(_stateMap.find(from) != _stateMap.end());
	assert(_stateMap.find(to) != _stateMap.end());
	_stateMap[from]->SetTransitionConditions(std::pair<std::string, std::shared_ptr<K3D12::StateNode>>(to, _stateMap.find(to)->second), conditions);

}

void K3D12::StateMachineController::ApplyRecursionMatirixBone(K3D12::MMDBoneNode* node)
{
	for (auto& nodeChildren : node->childrenBone) {
		_skelton.lock()->bonesMatrix[nodeChildren.index] *= _skelton.lock()->bonesMatrix[nodeChildren.parentIndex];
	}
	for (auto& nodeChildren : node->childrenBone) {
		ApplyRecursionMatirixBone(&nodeChildren);
	}
}