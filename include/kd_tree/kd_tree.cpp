#include "kd_tree.h"

/*
Description: K-D Tree improved for a better data structure performance. 
Author: Yixi Cai
email: yixicai@connect.hku.hk
*/

KD_TREE::KD_TREE(float delete_param, float balance_param, float box_length, int max_point_num) {
    delete_criterion_param = delete_param;
    balance_criterion_param = balance_param;
    downsample_size = box_length;
    Maximal_Point_Num = max_point_num;    
}

KD_TREE::~KD_TREE()
{
    Delete_Storage_Disabled = true;
    delete_tree_nodes(Root_Node, Points_deleted);
    Delete_Storage_Disabled = false;    
    PointVector ().swap(PCL_Storage);
    PointVector ().swap(Points_deleted);
    PointVector ().swap(Downsample_Storage);
}

void KD_TREE::Set_delete_criterion_param(float delete_param){
    delete_criterion_param = delete_param;
}

void KD_TREE::Set_balance_criterion_param(float balance_param){
    balance_criterion_param = balance_param;
}

void KD_TREE::Build(PointVector point_cloud){
    if (Root_Node != nullptr){
        Delete_Storage_Disabled = true;
        delete_tree_nodes(Root_Node, Points_deleted);
        Delete_Storage_Disabled = false;
    }
    rebuild_counter = 0;  
    BuildTree(Root_Node, 0, point_cloud.size()-1, point_cloud);
}

void KD_TREE::Nearest_Search(PointType point, int k_nearest, PointVector& Nearest_Points){
    priority_queue<PointType_CMP> q; // the priority queue;
    search_counter = 0;
    Search(Root_Node, k_nearest, point, q);
    // printf("Search Counter is %d ",search_counter);
    int k_found = min(k_nearest,int(q.size()));
    for (int i=0;i < k_found;i++){
        Nearest_Points.insert(Nearest_Points.begin(), q.top().point);
        q.pop();
    }
    return;
}

void KD_TREE::Add_Points(PointVector & PointToAdd){
    rebuild_counter = 0;
    BoxPointType Box_of_Point;
    PointType downsample_result, mid_point;
    float min_dist, tmp_dist;
    for (int i=0; i<PointToAdd.size();i++){
        // Downsample Before Add
        Box_of_Point.vertex_min[0] = floor(PointToAdd[i].x/downsample_size)*downsample_size;
        Box_of_Point.vertex_max[0] = Box_of_Point.vertex_min[0]+downsample_size;
        Box_of_Point.vertex_min[1] = floor(PointToAdd[i].y/downsample_size)*downsample_size;
        Box_of_Point.vertex_max[1] = Box_of_Point.vertex_min[1]+downsample_size; 
        Box_of_Point.vertex_min[2] = floor(PointToAdd[i].z/downsample_size)*downsample_size;
        Box_of_Point.vertex_max[2] = Box_of_Point.vertex_min[2]+downsample_size; 
        // printf("%0.3f %0.3f %0.3f-------------------------------------\n", PointToAdd[i].x, PointToAdd[i].y, PointToAdd[i].z);
        // printf("%0.3f %0.3f %0.3f %0.3f %0.3f %0.3f ------------------\n", Box_of_Point.vertex_min[0], Box_of_Point.vertex_max[0],Box_of_Point.vertex_min[1], Box_of_Point.vertex_max[1],Box_of_Point.vertex_min[2], Box_of_Point.vertex_max[2]) ; 
        mid_point.x = (Box_of_Point.vertex_max[0]-Box_of_Point.vertex_min[0])/2.0;
        mid_point.y = (Box_of_Point.vertex_max[1]-Box_of_Point.vertex_min[1])/2.0;
        mid_point.z = (Box_of_Point.vertex_max[2]-Box_of_Point.vertex_min[2])/2.0;
        PointVector ().swap(Downsample_Storage);
        Delete_by_range(Root_Node, Box_of_Point, true);                    
        min_dist = calc_dist(PointToAdd[i],mid_point);
        downsample_result = PointToAdd[i];
        for (int index = 0; index < Downsample_Storage.size(); index++){
            tmp_dist = calc_dist(Downsample_Storage[index], mid_point);
            if (tmp_dist < min_dist){
                min_dist = tmp_dist;
                downsample_result = Downsample_Storage[index];
            }
        }
        Add(Root_Node, downsample_result);        
    }
    printf("Rebuild counter is %d, ", rebuild_counter);   
    return;
}

void KD_TREE::Delete_Points(PointVector & PointToDel){
    bool flag;
    rebuild_counter = 0;
    for (int i=0;i!=PointToDel.size();i++){
        flag = Delete_by_point(Root_Node, PointToDel[i]);
        if (!flag) {
            printf("Failed to delete point (%0.3f,%0.3f,%0.3f)\n", PointToDel[i].x, PointToDel[i].y, PointToDel[i].z);
        }
    }
    printf("Rebuild counter is %d, ", rebuild_counter);      
    return;
}

void KD_TREE::Delete_Point_Boxes(vector<BoxPointType> & BoxPoints){
    rebuild_counter = 0;
    delete_counter = 0;
    for (int i=0;i < BoxPoints.size();i++){
        Delete_by_range(Root_Node ,BoxPoints[i], false);
    }
    printf("Delete counter is %d, ", delete_counter);
    printf("Rebuild counter is %d, ", rebuild_counter);     
    return;
}

void KD_TREE::acquire_removed_points(PointVector & removed_points){
    removed_points = Points_deleted;
    PointVector ().swap(Points_deleted);
    return;
}

void KD_TREE::BuildTree(KD_TREE_NODE * &root, int l, int r, PointVector & Storage){
    if (l>r) return;
    KD_TREE_NODE * new_node = new KD_TREE_NODE;
    int mid = (l+r)>>1; 
    // Find the best division Axis
    int i;
    float average[3] = {0,0,0};
    float covariance[3] = {0,0,0};
    for (i=l;i<=r;i++){
        average[0] += Storage[i].x;
        average[1] += Storage[i].y;
        average[2] += Storage[i].z;
    }
    for (i=0;i<3;i++) average[i] = average[i]/(r-l+1);
    for (i=l;i<=r;i++){
        covariance[0] += (Storage[i].x - average[0]) * (Storage[i].x - average[0]);
        covariance[1] += (Storage[i].y - average[1]) * (Storage[i].y - average[1]);  
        covariance[2] += (Storage[i].z - average[2]) * (Storage[i].z - average[2]);              
    }
    for (i=0;i<3;i++) covariance[i] = covariance[i]/(r-l+1);    
    int div_axis = 0;
    for (i = 1;i<3;i++){
        if (covariance[i] > covariance[div_axis]) div_axis = i;
    }
    new_node->division_axis = div_axis;
    switch (div_axis)
    {
    case 0:
        nth_element(begin(Storage)+l, begin(Storage)+mid, begin(Storage)+r+1, point_cmp_x);
        break;
    case 1:
        nth_element(begin(Storage)+l, begin(Storage)+mid, begin(Storage)+r+1, point_cmp_y);
        break;
    case 2:
        nth_element(begin(Storage)+l, begin(Storage)+mid, begin(Storage)+r+1, point_cmp_z);
        break;
    default:
        nth_element(begin(Storage)+l, begin(Storage)+mid, begin(Storage)+r+1, point_cmp_x);
        break;
    }  
    new_node->point = Storage[mid];             
    BuildTree(new_node->left_son_ptr, l, mid-1, Storage);
    BuildTree(new_node->right_son_ptr, mid+1, r, Storage);  
    Update(new_node);
    root = new_node;    
    return;
}

void KD_TREE::Rebuild(KD_TREE_NODE * &root){
    // Clear the PCL_Storage vector and release memory
    PointVector ().swap(PCL_Storage);
    rebuild_counter += root->TreeSize;    
    traverse_for_rebuild(root, PCL_Storage);
    delete_tree_nodes(root, Points_deleted);
    BuildTree(root, 0, PCL_Storage.size()-1, PCL_Storage);
    return;
}

void KD_TREE::Delete_by_range(KD_TREE_NODE * &root, BoxPointType boxpoint, bool is_downsample){
    Push_Down(root);     
    if (root == nullptr || root->tree_deleted) return;
    if (boxpoint.vertex_max[0] + EPS < root->node_range_x[0] || boxpoint.vertex_min[0] - EPS > root->node_range_x[1]) return;
    if (boxpoint.vertex_max[1] + EPS < root->node_range_y[0] || boxpoint.vertex_min[1] - EPS > root->node_range_y[1]) return;
    if (boxpoint.vertex_max[2] + EPS < root->node_range_z[0] || boxpoint.vertex_min[2] - EPS > root->node_range_z[1]) return;
    if (boxpoint.vertex_min[0]-EPS < root->node_range_x[0] && boxpoint.vertex_max[0]+EPS > root->node_range_x[1] && boxpoint.vertex_min[1]-EPS < root->node_range_y[0] && boxpoint.vertex_max[1]+EPS > root->node_range_y[1] && boxpoint.vertex_min[2]-EPS < root->node_range_z[0] && boxpoint.vertex_max[2]+EPS > root->node_range_z[1]){
        //printf("Delete in range (%0.3f,%0.3f) (%0.3f,%0.3f) (%0.3f,%0.3f)\n",x_range[0],x_range[1],y_range[0],y_range[1],z_range[0],z_range[1]);        
        //printf("Delete by range (%0.3f,%0.3f) (%0.3f,%0.3f) (%0.3f,%0.3f)\n",root->node_range_x[0],root->node_range_x[1],root->node_range_y[0],root->node_range_y[1],root->node_range_z[0],root->node_range_z[1]);
        root->tree_deleted = true;
        root->point_deleted = true;
        root->invalid_point_num = root->TreeSize;
        delete_counter += root->TreeSize;
        //printf("%d %d-----------------------------------------\n",int(Downsample_Storage.size()), root->TreeSize);  
        if (is_downsample) delete_tree_nodes(root, Downsample_Storage);
        //printf("%d %d-----------------------------------------\n",int(Downsample_Storage.size()), root->TreeSize); 
        return;
    }
    if (boxpoint.vertex_min[0]-EPS < root->point.x && boxpoint.vertex_max[0]+EPS > root->point.x && boxpoint.vertex_min[1]-EPS < root->point.y && boxpoint.vertex_max[1]+EPS > root->point.y && boxpoint.vertex_min[2]-EPS < root->point.z && boxpoint.vertex_max[2]+EPS > root->point.z){
        //printf("Deleted points: (%0.3f,%0.3f,%0.3f)\n",root->point.x,root->point.y,root->point.z);
        root->point_deleted = true;
        root->invalid_point_num += 1;
        delete_counter += 1;
        if (is_downsample) Downsample_Storage.push_back(root->point);        
    }    
    Delete_by_range(root->left_son_ptr, boxpoint, is_downsample);
    Delete_by_range(root->right_son_ptr, boxpoint, is_downsample);
    Update(root);
    root->need_rebuild = Criterion_Check(root);
    if (!(root->need_rebuild)){
        if (root->left_son_ptr != nullptr & root->left_son_ptr->need_rebuild) Rebuild(root->left_son_ptr);
        if (root->right_son_ptr != nullptr & root->right_son_ptr->need_rebuild) Rebuild(root->right_son_ptr);
    } else if (root == Root_Node) {
        Rebuild(root);
    }
    return;
}

bool KD_TREE::Delete_by_point(KD_TREE_NODE * &root, PointType point){
    Push_Down(root); 
    if (root == nullptr || root->tree_deleted) return false;
    bool flag = false;
    if (same_point(root->point, point) && !root->point_deleted) {
        root->point_deleted = true;
        root->invalid_point_num += 1;
        if (root->invalid_point_num == root->TreeSize) root->tree_deleted = true;      
        return true;
    }
    switch (root->division_axis)
    {
    case 0:
        if (point.x < root->point.x ) flag = Delete_by_point(root->left_son_ptr, point);
            else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    case 1:
        if (point.y < root->point.y) flag = Delete_by_point(root->left_son_ptr, point);
        else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    case 2:
        if (point.z < root->point.z) flag = Delete_by_point(root->left_son_ptr, point);
        else flag = Delete_by_point(root->right_son_ptr, point);
        break;
    default:
        if (point.x < root->point.x) flag = Delete_by_point(root->left_son_ptr, point);
            else flag = Delete_by_point(root->right_son_ptr, point);    
        break;
    }
    Update(root);
    root->need_rebuild = Criterion_Check(root);
    if (!root->need_rebuild){
        if (root->left_son_ptr != nullptr & root->left_son_ptr->need_rebuild) Rebuild(root->left_son_ptr);
        if (root->right_son_ptr != nullptr & root->right_son_ptr->need_rebuild) Rebuild(root->right_son_ptr);
    } else if (root == Root_Node) Rebuild(root);
    return flag;
}

void KD_TREE::Add(KD_TREE_NODE * &root, PointType point){
    Push_Down(root);     
    if (root == nullptr){
        root = new KD_TREE_NODE;
        root->point = point;
        Update(root);
        return;
    }
    switch (root->division_axis)
    {
    case 0:
        if (point.x < root->point.x) Add(root->left_son_ptr, point);
            else Add(root->right_son_ptr, point);
        break;
    case 1:
        if (point.y < root->point.y ) Add(root->left_son_ptr, point);
        else Add(root->right_son_ptr, point);
        break;
    case 2:
        if (point.z < root->point.z) Add(root->left_son_ptr, point);
        else Add(root->right_son_ptr, point);
        break;
    default:
        if (point.x < root->point.x) Add(root->left_son_ptr, point);
            else Add(root->right_son_ptr, point);    
        break;
    }    
    Update(root);
    root->need_rebuild = Criterion_Check(root);
    if (!root->need_rebuild){
        if (root->left_son_ptr != nullptr & root->left_son_ptr->need_rebuild) Rebuild(root->left_son_ptr);
        if (root->right_son_ptr != nullptr & root->right_son_ptr->need_rebuild) Rebuild(root->right_son_ptr);
    } else if (root == Root_Node) Rebuild(root);
    return;
}

void KD_TREE::Search(KD_TREE_NODE * root, int k_nearest, PointType point, priority_queue<PointType_CMP> & q){
    // Push_Down(root);
    if (root == nullptr || root->tree_deleted) return;
    search_counter += 1;
    if (!root->point_deleted){
        float dist = calc_dist(point, root->point);
        if (q.size() < k_nearest || dist < q.top().dist){
            if (q.size() >= k_nearest) q.pop();
            PointType_CMP current_point{root->point, dist};                    
            q.push(current_point);            
        }
    }
    float dist_left_node = calc_box_dist(root->left_son_ptr, point);
    float dist_right_node = calc_box_dist(root->right_son_ptr, point);
    if (q.size()< k_nearest || dist_left_node < q.top().dist && dist_right_node < q.top().dist){
        if (dist_left_node <= dist_right_node) {
            Search(root->left_son_ptr, k_nearest, point, q);
            if (q.size() < k_nearest || dist_right_node < q.top().dist) Search(root->right_son_ptr, k_nearest, point, q);
        } else {
            Search(root->right_son_ptr, k_nearest, point, q);
            if (q.size() < k_nearest || dist_left_node < q.top().dist) Search(root->left_son_ptr, k_nearest, point, q);
        }
    } else {
        if (dist_left_node < q.top().dist) Search(root->left_son_ptr, k_nearest, point, q);
        if (dist_right_node < q.top().dist) Search(root->right_son_ptr, k_nearest, point, q);
    }
    return;
}

bool KD_TREE::Criterion_Check(KD_TREE_NODE * root){
    if (root->TreeSize < Minimal_Unbalanced_Tree_Size){
        return false;
    }
    float balance_evaluation = 0.0f;
    float delete_evaluation = 0.0f;
    KD_TREE_NODE * son_ptr = root->left_son_ptr;
    if (son_ptr == nullptr) son_ptr = root->right_son_ptr;
    delete_evaluation = float(root->invalid_point_num)/ root->TreeSize;
    balance_evaluation = float(son_ptr->TreeSize) / root->TreeSize;
    if (delete_evaluation > delete_criterion_param){
        return true;
    }
    if (balance_evaluation > balance_criterion_param || balance_evaluation < 1-balance_criterion_param){
        return true;
    } 
    return false;
}

void KD_TREE::Push_Down(KD_TREE_NODE *root){
    if (root == nullptr) return;
    if (root->left_son_ptr != nullptr && root->tree_deleted) {
        root->left_son_ptr->point_deleted = true;
        root->left_son_ptr->tree_deleted = true;
    } 
    if (root->right_son_ptr != nullptr && root->tree_deleted){
        root->right_son_ptr->point_deleted = true;
        root->right_son_ptr->tree_deleted = true;
    }
    return;
}

void KD_TREE::Update(KD_TREE_NODE* root){
    KD_TREE_NODE * left_son_ptr = root->left_son_ptr;
    KD_TREE_NODE * right_son_ptr = root->right_son_ptr;
    // Update Tree Size
    if (left_son_ptr != nullptr && right_son_ptr != nullptr){
        root->TreeSize = left_son_ptr->TreeSize + right_son_ptr->TreeSize + 1;
        root->invalid_point_num = left_son_ptr->invalid_point_num + right_son_ptr->invalid_point_num + (root->point_deleted? 1:0);
        root->tree_deleted = left_son_ptr->tree_deleted && right_son_ptr->tree_deleted && root->point_deleted;
        root->node_range_x[0] = min(min(left_son_ptr->node_range_x[0],right_son_ptr->node_range_x[0]),root->point.x);
        root->node_range_x[1] = max(max(left_son_ptr->node_range_x[1],right_son_ptr->node_range_x[1]),root->point.x);
        root->node_range_y[0] = min(min(left_son_ptr->node_range_y[0],right_son_ptr->node_range_y[0]),root->point.y);
        root->node_range_y[1] = max(max(left_son_ptr->node_range_y[1],right_son_ptr->node_range_y[1]),root->point.y);        
        root->node_range_z[0] = min(min(left_son_ptr->node_range_z[0],right_son_ptr->node_range_z[0]),root->point.z);
        root->node_range_z[1] = max(max(left_son_ptr->node_range_z[1],right_son_ptr->node_range_z[1]),root->point.z);         
    } else if (left_son_ptr != nullptr){
        root->TreeSize = left_son_ptr->TreeSize + 1;
        root->invalid_point_num = left_son_ptr->invalid_point_num + (root->point_deleted?1:0);
        root->tree_deleted = left_son_ptr->tree_deleted && root->point_deleted;
        root->node_range_x[0] = min(left_son_ptr->node_range_x[0],root->point.x);
        root->node_range_x[1] = max(left_son_ptr->node_range_x[1],root->point.x);
        root->node_range_y[0] = min(left_son_ptr->node_range_y[0],root->point.y);
        root->node_range_y[1] = max(left_son_ptr->node_range_y[1],root->point.y); 
        root->node_range_z[0] = min(left_son_ptr->node_range_z[0],root->point.z);
        root->node_range_z[1] = max(left_son_ptr->node_range_z[1],root->point.z);               
    } else if (right_son_ptr != nullptr){
        root->TreeSize = right_son_ptr->TreeSize + 1;
        root->invalid_point_num = right_son_ptr->invalid_point_num + (root->point_deleted? 1:0);
        root->tree_deleted = right_son_ptr->tree_deleted && root->point_deleted;        
        root->node_range_x[0] = min(right_son_ptr->node_range_x[0],root->point.x);
        root->node_range_x[1] = max(right_son_ptr->node_range_x[1],root->point.x);
        root->node_range_y[0] = min(right_son_ptr->node_range_y[0],root->point.y);
        root->node_range_y[1] = max(right_son_ptr->node_range_y[1],root->point.y); 
        root->node_range_z[0] = min(right_son_ptr->node_range_z[0],root->point.z);
        root->node_range_z[1] = max(right_son_ptr->node_range_z[1],root->point.z);        
    } else {
        root->TreeSize = 1;
        root->invalid_point_num = (root->point_deleted? 1:0);
        root->tree_deleted = root->point_deleted;
        root->node_range_x[0] = root->point.x;
        root->node_range_x[1] = root->point.x;        
        root->node_range_y[0] = root->point.y;
        root->node_range_y[1] = root->point.y; 
        root->node_range_z[0] = root->point.z;
        root->node_range_z[1] = root->point.z;                 
    }
    return;
}

void KD_TREE::traverse_for_rebuild(KD_TREE_NODE * root, PointVector &Storage){
    if (root == nullptr || root->tree_deleted) return;
    if (!root->point_deleted) {
        Storage.push_back(root->point);
    }
    traverse_for_rebuild(root->left_son_ptr, Storage);
    traverse_for_rebuild(root->right_son_ptr, Storage);
    return;
}

void KD_TREE::delete_tree_nodes(KD_TREE_NODE * &root, PointVector & Delete_Storage){
    if (root == nullptr) return;
    delete_tree_nodes(root->left_son_ptr, Delete_Storage);
    delete_tree_nodes(root->right_son_ptr, Delete_Storage);
    if (!Delete_Storage_Disabled) Delete_Storage.push_back(root->point);
    delete root;
    root = nullptr;
    return;
}

bool KD_TREE::same_point(PointType a, PointType b){
    return (fabs(a.x-b.x) < EPS && fabs(a.y-b.y) < EPS && fabs(a.z-b.z) < EPS );
}

float KD_TREE::calc_dist(PointType a, PointType b){
    float dist = 0.0f;
    dist = (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z);
    return dist;
}

float KD_TREE::calc_box_dist(KD_TREE_NODE * node, PointType point){
    if (node == nullptr) return INFINITY;
    float min_dist = 0.0;
    if (point.x < node->node_range_x[0]) min_dist += (point.x - node->node_range_x[0])*(point.x - node->node_range_x[0]);
    if (point.x > node->node_range_x[1]) min_dist += (point.x - node->node_range_x[1])*(point.x - node->node_range_x[1]);
    if (point.y < node->node_range_y[0]) min_dist += (point.y - node->node_range_y[0])*(point.y - node->node_range_y[0]);
    if (point.y > node->node_range_y[1]) min_dist += (point.y - node->node_range_y[1])*(point.y - node->node_range_y[1]);
    if (point.z < node->node_range_z[0]) min_dist += (point.z - node->node_range_z[0])*(point.z - node->node_range_z[0]);
    if (point.z > node->node_range_z[1]) min_dist += (point.z - node->node_range_z[1])*(point.z - node->node_range_z[1]);
    return min_dist;
}

bool KD_TREE::point_cmp_x(PointType a, PointType b) { return a.x < b.x;}
bool KD_TREE::point_cmp_y(PointType a, PointType b) { return a.y < b.y;}
bool KD_TREE::point_cmp_z(PointType a, PointType b) { return a.z < b.z;}
